#!/usr/bin/env python3
"""PreToolUse hook: only let Claude run ansible-playbook in modes that never
connect to a host.

The Bash command is split into shell words and operators (quotes, escapes,
comments and command separators are honoured). Every ansible-playbook
invocation must then carry one of SAFE_FLAGS as a real option argument, so
these are all denied:

    ansible-playbook site.yml # --syntax-check      (comment)
    ansible-playbook site.yml -e 'x=--syntax-check' (option value)
    ansible-playbook site.yml -e --syntax-check     (value of -e)
    ansible-playbook site.yml -- --syntax-check     (after end of options)
    ansible-playbook site.yml; echo --syntax-check  (a different command)

Anything the hook cannot analyse is also denied: unbalanced quotes, or
"ansible-playbook" appearing inside a larger word (bash -c '...', $(...),
backticks, ...). --check and --diff still SSH to the hosts and run modules,
so they are denied, and so are abbreviated flags such as --syntax.

This guards against mistakes, not against deliberate obfuscation.
"""

import json
import os
import sys

SAFE_FLAGS = frozenset(
    ("--syntax-check", "--list-tasks", "--list-tags", "--list-hosts")
)

# ansible-playbook options that consume the next argument as their value
# (from the ansible-core argument parser). A safe flag in that position is a
# value, not a mode switch.
OPTIONS_WITH_VALUE = frozenset((
    "-M", "-T", "-c", "-e", "-f", "-i", "-l", "-t", "-u",
    "--become-method", "--become-pass-file", "--become-password-file",
    "--become-user", "--conn-pass-file", "--connection",
    "--connection-password-file", "--extra-vars", "--forks", "--inventory",
    "--inventory-file", "--key-file", "--limit", "--module-path",
    "--private-key", "--scp-extra-args", "--sftp-extra-args", "--skip-tags",
    "--ssh-common-args", "--ssh-extra-args", "--start-at-task", "--tags",
    "--timeout", "--user", "--vault-id", "--vault-pass-file",
    "--vault-password-file",
))

# Characters that end a word and separate or redirect commands.
OPERATOR_CHARS = set(";&|()<>`\n")
BLANKS = set(" \t\r")

PROGRAM = "ansible-playbook"


class Ambiguous(Exception):
    """The command cannot be analysed safely."""


def tokenize(command):
    """Split a shell command into ("word", text) and ("op", char) tokens.

    Handles single and double quotes, backslash escapes and comments that
    start at the beginning of a word, close enough to bash for this purpose.
    Raises Ambiguous on unbalanced quotes or a trailing backslash.
    """
    tokens = []
    word = None  # None means "not inside a word"
    i, n = 0, len(command)

    def end_word():
        nonlocal word
        if word is not None:
            tokens.append(("word", word))
            word = None

    while i < n:
        c = command[i]
        if c in BLANKS:
            end_word()
        elif c in OPERATOR_CHARS:
            end_word()
            tokens.append(("op", c))
        elif c == "#" and word is None:
            # Comment to end of line; the newline itself still separates.
            while i < n and command[i] != "\n":
                i += 1
            continue
        elif c == "\\":
            if i + 1 >= n:
                raise Ambiguous("trailing backslash")
            if command[i + 1] != "\n":  # backslash-newline is a continuation
                word = (word or "") + command[i + 1]
            i += 1
        elif c == "'":
            end = command.find("'", i + 1)
            if end == -1:
                raise Ambiguous("unbalanced single quote")
            word = (word or "") + command[i + 1:end]
            i = end
        elif c == '"':
            buf = []
            i += 1
            while i < n and command[i] != '"':
                if command[i] == "\\" and i + 1 < n and command[i + 1] in '"\\$`':
                    i += 1
                buf.append(command[i])
                i += 1
            if i >= n:
                raise Ambiguous("unbalanced double quote")
            word = (word or "") + "".join(buf)
        else:
            word = (word or "") + c
        i += 1
    end_word()
    return tokens


def is_safe_invocation(args):
    """True if the arguments of one ansible-playbook call select a safe mode."""
    skip_value = False
    for arg in args:
        if skip_value:
            skip_value = False
        elif arg == "--":
            return False  # everything after this is a playbook path
        elif arg in SAFE_FLAGS:
            return True
        elif arg in OPTIONS_WITH_VALUE:
            skip_value = True
    return False


def check(command):
    """Return None if the command is allowed, else a reason for denying it."""
    if PROGRAM not in command:
        return None
    try:
        tokens = tokenize(command)
    except Ambiguous as exc:
        return "could not parse the command (%s)" % exc

    for index, (kind, text) in enumerate(tokens):
        if kind != "word" or PROGRAM not in text:
            continue
        if os.path.basename(text) != PROGRAM:
            return (
                "%r contains %s inside a larger word (bash -c, $(...), "
                "quoting, ...), which this hook cannot analyse" % (text, PROGRAM)
            )
        args = []
        for kind2, text2 in tokens[index + 1:]:
            if kind2 == "op":
                break
            args.append(text2)
        if not is_safe_invocation(args):
            return "%s is called without a safe mode option" % PROGRAM
    return None


def main():
    command = json.load(sys.stdin).get("tool_input", {}).get("command", "")
    reason = check(command)
    if reason is None:
        return
    json.dump(
        {
            "hookSpecificOutput": {
                "hookEventName": "PreToolUse",
                "permissionDecision": "deny",
                "permissionDecisionReason": (
                    "Blocked by .claude/hooks/guard_ansible_playbook.py: "
                    + reason
                    + ". ansible-playbook may only run with one of "
                    + ", ".join(sorted(SAFE_FLAGS))
                    + " as a standalone option. Running it any other way "
                    "(including --check or --diff) connects to inventory "
                    "hosts. Give the user the command to run themselves "
                    "instead."
                ),
            }
        },
        sys.stdout,
    )


if __name__ == "__main__":
    main()
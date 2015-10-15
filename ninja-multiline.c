#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int do_child(int argc, char *argv[]) {
    char *args[argc + 1];
    memcpy(args, argv, argc * sizeof(char*));
    args[argc] = NULL;
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(args[0], args);
}

int wait_for_syscall(pid_t child) {
    int status;

    while (1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);

        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;

        if (WIFEXITED(status))
            return 1;
    }
}

int do_parent(pid_t child) {
    int status;
    waitpid(child, &status, 0);
    ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);

    while (1) {
        if (wait_for_syscall(child))
            break;

        int syscall = ptrace(PTRACE_PEEKUSER, child, sizeof(long) * ORIG_EAX);
        fprintf(stderr, "syscall: %d = ", syscall);

        if (wait_for_syscall(child))
            break;

        int retval = ptrace(PTRACE_PEEKUSER, child, sizeof(long) * EAX);
        Fprintf(stderr, "%d\n", retval);
    }
}

int main(int argc, char *argv[]) {
    pid_t child = fork();

    if (child == 0) {
        return do_child(argc, argv);
    } else {
        return do_parent(child);
    }

    return 0;
}


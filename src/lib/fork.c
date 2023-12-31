//
// Process creation routines
//
#include <os.h>
#include <string.h>
#include <unistd.h>
#include <atomic.h>
#include <crtbase.h>
#include <sys/wait.h>

static int forkpid = 0;

//
// This implementation of vfork()/exec() has been inspired by Shaun 
// Jackman's <sjackman@gmail.com> implementation for BusyBox. It uses 
// setjmp()/longjmp() to emulate the behaviour of vfork()/exec().
//

static char **copyenv(char **env) {
    int n;
    char **newenv;

    if (!env) return NULL;
    for (n = 0; env[n]; n++);
    newenv = (char **) malloc((n + 1) * sizeof(char *));
    if (newenv) {
        newenv[n] = NULL;
        for (n = 0; env[n]; n++) newenv[n] = strdup(env[n]);
    }

    return newenv;
}

static void freeenv(char **env) {
    int n;

    if (!env) return;
    for (n = 0; env[n]; n++) free(env[n]);
    free(env);
}

static resume_fork(struct tib *tib, int pid) {
    int i;
    struct process *proc = tib->proc;
    struct _forkctx *fc = (struct _forkctx *) tib->forkctx;

    // Restore standard handles
    for (i = 0; i < 3; i++) {
        close(proc->iob[i]);
        proc->iob[i] = fc->fd[i];
    }

    // Restore environment variables
    freeenv(proc->env);
    proc->env = fc->env;

    // Unlink fork context from chain
    tib->forkctx = fc->prev;

    // Return control to the parent process path with pid handle
    longjmp(fc->jmp, pid);
}

void fork_exit(int status) {
    struct tib *tib = gettib();

    // If no vfork() is in progress just return
    struct _forkctx *fc = (struct _forkctx *) tib->forkctx;
    if (!fc) return;

    // Add a fake zombie for process
    setchildstat(0x40000000 | fc->pid, status & 0xFF);

    // Send a SIGCHLD to account for the virtual process exit
    sendsig(self(), SIGCHLD);

    // Return control to the parent process path with fork pid
    resume_fork(tib, 0x40000000 | fc->pid);
}

struct _forkctx *_vfork(struct _forkctx *fc) {
    int i;
    struct tib *tib = gettib();
    struct process *proc = tib->proc;
    struct crtbase *crtbase = (struct crtbase *) proc->crtbase;

    // Assign fork pid
    fc->pid = atomic_increment(&forkpid);

    // Install exit handler.
    crtbase->fork_exit = fork_exit;

    // Save and duplicate standard handles
    for (i = 0; i < 3; i++) {
        fc->fd[i] = proc->iob[i];
        proc->iob[i] = dup(proc->iob[i]);
    }

    // Save and duplicate environment variables
    fc->env = proc->env;
    proc->env = copyenv(proc->env);

    // Link fork context into chain
    fc->prev = tib->forkctx;
    tib->forkctx = fc;

    return fc;
}

pid_t wait(int *stat_loc) {
    struct process *proc = gettib()->proc;

    while (1) {
        int pid = getchildstat(-1, stat_loc);
        if (pid != -1) return pid;
        sigsuspend(NULL);
    }
}

pid_t waitpid(pid_t pid, int *stat_loc, int options) {
    struct process *proc = gettib()->proc;
    int rc;
    handle_t h;

    // Use wait() if pid is -1 (TODO pid==0 meaning wait for any child in process group)
    if (pid <= 0) {
        if (options & WNOHANG) {
            return getchildstat(-1, stat_loc);
        } else {
            return wait(stat_loc);
        }
    }

    while (1) {
        // Check for zombies
        rc = getchildstat(pid, stat_loc);
        if (rc != -1 || (options & WNOHANG)) return rc;

        // Get handle for process from pid
        h = getprochandle(pid);
        if (h == NOHANDLE) {
            errno = ECHILD;
            return -1;
        }

        // Wait for main thread in process to terminate
        rc = waitone(h, INFINITE);
        close(h);
    }
}

int execve(const char *path, char *argv[], char *env[]) {
    struct tib *tib = gettib();
    struct process *proc = tib->proc;
    int pid;
    handle_t child;
    struct tib *ctib;

    // If no vfork() is in progress return error
    struct _forkctx *fc = (struct _forkctx *) tib->forkctx;
    if (!fc) {
        errno = EPERM;
        return -1;
    }

    // Spawn new child process
    child = spawnve(P_SUSPEND | P_CHILD, path, argv, env, &ctib);
    if (child < 0) return -1;
    pid = ctib->proc->id;
    resume(child);
    close(child);

    // Return control to the parent process path with pid
    resume_fork(tib, pid);
    return 0;
}

int execv(const char *path, char *argv[]) {
    return execve(path, argv, NULL);
}

int execl(const char *path, char *arg0, ...) {
    return execve(path, &arg0, NULL);
}

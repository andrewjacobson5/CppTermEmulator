#include <iostream>
#include <cstring>
#include <pty.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>

#define bufSize 2048


int terminalOperations(){
     int master, slave;
    char buf[bufSize];

    std::cout << "Opening PTY..." << std::endl;
    if (openpty(&master, &slave, NULL, NULL, NULL) < 0) {
        perror("openpty");
        return 1;
    }

    struct termios tty_attrs;
        if (tcgetattr(slave, &tty_attrs) < 0) {
            perror("tcgetattr");
            exit(1); //don't return we are in child process
        }

        tty_attrs.c_lflag &= ~ECHO; // Disable echo

        if (tcsetattr(slave, TCSANOW, &tty_attrs) < 0) {
            perror("tcsetattr");
            exit(1);
        }


    std::cout << "Forking process..." << std::endl;
    pid_t pid = fork(); // make a new fork
    if (pid == -1) { // error handling
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // Child process (slave)
        close(master);
        std::cout << "Child process running..." << std::endl;

        // Create a new session and set the slave as the controlling terminal
        if (setsid() < 0) {
            perror("setsid");
            return 1;
        }
        if (ioctl(slave, TIOCSCTTY, NULL) < 0) {  // i/o control, returns -1 on error 
            perror("ioctl");
            return 1;
        }

        // Duplicate the slave to stdin, stdout, and stderr
        if (dup2(slave, STDIN_FILENO) < 0) {
            perror("dup2 stdin");
            return 1;
        }
        if (dup2(slave, STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
            return 1;
        }
        if (dup2(slave, STDERR_FILENO) < 0) {
            perror("dup2 stderr");
            return 1;
        }

        // Close the slave end
        close(slave);

        std::cout << "Executing shell..." << std::endl;

        // Replace the child process with the shell
        execlp("/bin/bash", "/bin/bash", NULL); // executes the shell within the child process

        // If execlp fails
        perror("execlp");
        return 1;
    } else {
        // Parent process (master)
        close(slave);
        std::cout << "Parent process running..." << std::endl;

        fd_set read_fds;
        while (true) {
            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);
            FD_SET(master, &read_fds);

            // Wait for input from stdin or the PTY master
            if (select(master + 1, &read_fds, NULL, NULL, NULL) < 0) {
                perror("select");
                break;
            }

            // Input from stdin to PTY master
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                ssize_t nbytes = read(STDIN_FILENO, buf, sizeof(buf));
                if (nbytes <= 0) break;
                write(master, buf, nbytes);
                fsync(master);  // Ensure data is flushed to the master PTY
            }

            // Output from PTY master to stdout
            if (FD_ISSET(master, &read_fds)) {
                ssize_t nbytes = read(master, buf, sizeof(buf));
                if (nbytes <= 0) break;
                write(STDOUT_FILENO, buf, nbytes);
                fsync(STDOUT_FILENO);  // Ensure data is flushed to the stdout
            }
        }

        // Wait for the child process to terminate
        int status;
        waitpid(pid, &status, 0);
    }

    std::cout << "Program exiting..." << std::endl;
    return 0;
}
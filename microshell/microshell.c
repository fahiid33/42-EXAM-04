//----------------------------//
//    passed moulinette       //
//----------------------------//

#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

int	ft_error(char *err, char *arg)
{
	while (*err)
		write(2, err++, 1);
	if (arg)
		while(*arg)
			write(2, arg++, 1);
	write(2, "\n", 1);
	return (1);
}

int exec(char **av, int i, int tmp_fd, char **env)
{
	av[i] = NULL;
	if (dup2(tmp_fd, 0) < 0)
		ft_error("error: fatal\n", NULL);
	close(tmp_fd);
	execve(av[0], av, env);
	return (ft_error("error: cannot execute ", av[0]));
}

int	main(int ac, char *av[], char *env[])
{
	int	i;
	int fd[2];
	int tmp_fd;
	(void)ac;

	i = 0;
	tmp_fd = dup(0);
	int	pid;
	while (av[i] && av[i + 1])
	{
		av = &av[i + 1];
		i = 0;
		while (av[i] && strcmp(av[i], ";") && strcmp(av[i], "|"))
			i++;
		if (strcmp(av[0], "cd") == 0)
		{
			if (i != 2)
				ft_error("error: cd: bad arguments", NULL);
			else if (chdir(av[1]) != 0)
				ft_error("error: cd: cannot change directory to ", av[1]	);
		}
		else if (i != 0 && (av[i] == NULL || strcmp(av[i], ";") == 0))
		{
			pid = fork();
			if (pid < 0)
				ft_error("error: fatal\n", NULL);
			if (pid == 0)
			{
				if (exec(av, i, tmp_fd, env))
					return (1);
			}
			else
			{
				close(tmp_fd);
				while(waitpid(-1, NULL, WUNTRACED) != -1)
					;
				tmp_fd = dup(0);
			}
		}
		else if(i != 0 && strcmp(av[i], "|") == 0)
		{
			if (pipe(fd) < 0)
                ft_error("error: fatal\n", NULL);
			pid = fork();
			if (pid < 0)
				ft_error("error: fatal\n", NULL);
			if (pid == 0)
			{
				if (dup2(fd[1], 1) < 0)
					ft_error("error: fatal\n", NULL);
				close(fd[0]);
				close(fd[1]);
				if (exec(av, i, tmp_fd, env))
					return (1);
			}
			else
			{
				close(fd[1]);
				close(tmp_fd);
				tmp_fd = fd[0];
			}
		}
	}
	close(tmp_fd);
	return (0);
}
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define TYPE_PIPE 1
#define TYPE_END 2
#define TYPE_BREAK 3


typedef struct s_data
{
	char **argv;
	int		type;
	int		size;
	int		fd[2];
	struct s_data *next;
	struct s_data *prev;
}	t_data;

//utils

int	ft_strlen(char *str)
{
	int i = 0;
	if (!str)
		return (0);
	while (str[i])
		i++;
	return i;
}
//errors

void	exit_fatal(void)
{
	write(2, "error: fatal\n", 14);
	exit(EXIT_FAILURE);
}
void	exit_cd1(void)
{
	write(2, "error: cd: bad arguments\n", 26);
	exit(EXIT_FAILURE);
}
void	exit_cd2(char *str)
{
	write(2, "error: cd: cannot change directory to ", 39);
	write(2, str, ft_strlen(str));
	write(2, "\n", 1);
	exit(EXIT_FAILURE);
}

void	exec_error(char *cmd)
{
	write(2, "error: cannot execute ", 23);
	write(2, cmd, ft_strlen(cmd));
	write(2, "\n", 1);
	exit(EXIT_FAILURE);
}
char    *ft_strdup(char *str)
{
    int	i;
	i = 0;
	char *dup;
	if (!str)
		return (NULL);
	dup = malloc(sizeof(char) * ft_strlen(str) + 1);
	if (!dup)
		exit_fatal();
	while (str[i])
	{
		dup[i] = str[i];
		i++;
	}
	dup[i] = '\0';
	return (dup);
}
int	size_argv(char **av)
{
	int	i = 0;
	while (av && av[i] && strcmp(av[i], ";") != 0 && strcmp(av[i], "|") != 0)
		i++;
	return (i);
}
int	check_end(char *arg)
{
	if (!arg)
		return(TYPE_END);
	if (strcmp(arg, "|") == 0)
		return (TYPE_PIPE);
	if (strcmp(arg, ";") == 0)
		return (TYPE_BREAK);
	return (0);
}
void	lst_addback(t_data **lst, t_data *new)
{
	t_data *temp;
	if (!(*lst))
		*lst = new;
	else
	{
		temp = *lst;
		while (temp->next)
			temp = temp->next;
		temp->next = new;
		new->prev = temp;
	}
}
int	parse_argv(t_data **lst, char **av)
{
	t_data *new;
	int	size = size_argv(av);
	new = malloc(sizeof(t_data));
	if (!new)
		exit_fatal();
	new->argv = malloc(sizeof(char *) * size + 1);
	if (!new->argv)
		exit_fatal();
	new->size = size;
	new->next = NULL;
	new->prev = NULL;
	new->argv[size] = NULL;
	while (--size >= 0)
	{
		new->argv[size] = ft_strdup(av[size]);
	}
	new->type = check_end(new->argv[new->size]);
	lst_addback(lst, new);
	return (new->size);
}

//execute

void	exec_cmd(t_data *data, char **env)
{
	t_data *temp;
	temp = data;
	int	pid;
	int	status;
	int	pipe_open = 0;
	if (temp->type == TYPE_PIPE || (temp->prev && temp->prev->type == TYPE_PIPE))
	{
		pipe_open = 1;
		if (pipe(temp->fd))
			exit_fatal();
	}
	pid = fork();
	if (pid < 0)
		exit_fatal();
	if (pid == 0)
	{
		if (temp->type == TYPE_PIPE && dup2(temp->fd[1], 1) < 0)
			exit_fatal();
		else if (temp->prev && temp->prev->type == TYPE_PIPE && dup2(temp->fd[0], 0) < 0)
			exit_fatal();
		if (execve(temp->argv[0], temp->argv, env) < 0)
			exec_error(temp->argv[0]);
		exit(EXIT_SUCCESS);
	}
	else
	{
		waitpid(pid, &status, 0);
		if (pipe_open)
		{
			close(temp->fd[1]);
			if (!temp->next || temp->type == TYPE_BREAK)
				close(temp->fd[0]);
			if (temp->prev && temp->prev->type == TYPE_PIPE)
				close(temp->fd[0]);
		}
	}
}
void	exec_cmds(t_data *data, char **env)
{
	t_data *tmp;
	tmp = data;
	
	while (tmp)
	{
		if (strcmp(tmp->argv[0], "cd") == 0)
		{
			if (tmp->size < 2)
				exit_cd1();
			else if (chdir(tmp->argv[1]))
				exit_cd2(tmp->argv[1]);
		}
		else
			exec_cmd(tmp, env);
		tmp = tmp->next;
	}
}

void free_all(t_data *ptr)
{
	t_data *temp;
	int i;

	while (ptr)
	{
		temp = ptr->next;
		i = 0;
		while (i < ptr->size)
			free(ptr->argv[i++]);
		free(ptr->argv);
		free(ptr);
		ptr = temp;
	}
	ptr = NULL;
}

int main(int ac, char **av, char **env)
{
	t_data *data = NULL;
	int	i = 1;
	if (ac > 1)
	{
		while (av[i])
		{
			if (!strcmp(av[i], ";"))
			{
				i++;
				continue ;
			}
			i += parse_argv(&data, &av[i]);
			if (!av[i])
				break ;
			else
				i++;
		}
		if (data)
			exec_cmds(data, env);
		free_all(data);
	}
}
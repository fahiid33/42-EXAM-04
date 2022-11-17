#include <stdio.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>


#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define TYPE_PIPE 1
#define TYPE_BREAK 2
#define TYPE_END 3


typedef struct s_data
{
    char    **argv;
    int     fd[2];
    int     size;
    int     type;
    struct s_data *next;
    struct s_data *prev;
}   t_data;

// utils
int ft_strlen(char *str)
{
    int i = 0;
    if (!str)
        return (0);
    while (str[i])
        i++;
    return(i);
}

char    *ft_strdup(char *s)
{
    char    *dup;
    int i = 0;
    if (!s)
        return (NULL);
    dup = malloc(sizeof(char) * ft_strlen(s) + 1);
    while (s[i])
    {
        dup[i] = s[i];
        i++;
    }
    dup[i] = '\0';
    return (dup);
}

void    lst_addback(t_data **lst, t_data *new)
{
    t_data *tmp;

    if (!(*lst))
        *lst = new;
    else
    {
        tmp = *lst;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = new;
        new->prev = tmp;
    }    
}
int argv_size(char **av)
{
    int i = 0;
    while (av[i] && strcmp(av[i], ";") != 0 && strcmp(av[i], "|") != 0)
        i++;
    return (i);
}
int check_end(char *av)
{
    if (!av)
        return(TYPE_END);
    if (!strcmp(av, ";"))
        return (TYPE_BREAK);
    if (!strcmp(av, "|"))
        return (TYPE_PIPE);
    return (0);
}

// errors

void    cd_error1(void)
{
    write(STDERR, "error: cd: bad arguments\n", 26);
    exit(EXIT_FAILURE);
}
void    cd_error2(char *str)
{
     write(STDERR, "error: cd: cannot change directory to ", 39);
     write(STDERR, str, ft_strlen(str));
     write(STDERR, "\n", 1);
     exit(EXIT_FAILURE);
}
void    exit_fatal(void)
{
    write(STDERR, "error: fatal\n", 14);
    exit(EXIT_FAILURE);
}
void    exec_error(char *cmd)
{
    write(STDERR, "error: cannot execute ", 23);
    write(STDERR, cmd, ft_strlen(cmd));
    write(STDERR, "\n", 1);
    exit(EXIT_FAILURE);
}

// part parse
int parse_argv(t_data **ptr, char **av)
{
    int     size = argv_size(av);
    t_data *new;
    new = malloc(sizeof(t_data));
    new->size = size;
    new->argv = malloc(sizeof(char *) * size + 1);
    new->argv[size] = NULL;
    new->next = NULL;
    new->prev = NULL;
    while (--size >= 0)
    {
        new->argv[size] = ft_strdup(av[size]);
    }
    new->type = check_end(av[new->size]);
    lst_addback(ptr, new);
    return (new->size);
}

void    exec_cmd(t_data *tmp, char **env)
{
    int pipe_open;
    int status;
    int pid;
    if (tmp->type == TYPE_PIPE || (tmp->prev && tmp->prev->type == TYPE_PIPE))
    {
        pipe_open = 1;
        if (pipe(tmp->fd) == -1)
            exit_fatal();
    }
    pid = fork();
    if (pid < 0)
        exit_fatal();
    else if (pid == 0)
    {
        if (tmp->type == TYPE_PIPE && dup2(tmp->fd[STDOUT], STDOUT) < 0)
            exit_fatal();
        if (tmp->prev && tmp->prev->type == TYPE_PIPE && dup2(tmp->fd[STDIN], STDIN))
            exit_fatal();  
        if (execve(tmp->argv[0], tmp->argv, env) < 0)
            exec_error(tmp->argv[0]);
        exit(EXIT_SUCCESS);
    }
    else
    {
        waitpid(-1, &status, 0);
        if (pipe_open)
        {
            close(tmp->fd[STDOUT]);
            if (!tmp->next ||  tmp->type == TYPE_BREAK)
                close(tmp->fd[STDIN]);
        }
        if (tmp->prev && tmp->prev->type == TYPE_PIPE)
            close(tmp->prev->fd[STDIN]);
    }
}

void    exec_cmds(t_data *data, char **env)
{
    t_data *tmp;
    tmp = data;
    while (tmp)
    {
        if (!strcmp(tmp->argv[0], "cd"))
        {
            if (tmp->size < 2)
                cd_error1();
            else if (chdir(tmp->argv[1]))
                cd_error2(tmp->argv[1]);
        }
        else
            exec_cmd(tmp, env);
        tmp = tmp->next;
    }
}
void    free_list(t_data *data)
{
    t_data *tmp;
    int i = 0;
    while (data)
    {
        tmp = data->next;
        while (i < data->size)
            free(data->argv[i++]);
        free(data->argv);
        free(data);
        data = tmp;
    }
}

int main(int ac , char **av, char **env)
{
    int i;
    t_data *tmp;

    tmp = NULL;
    i = 1;
    if (ac > 1)
    {
        while (av[i])
        {
            if (!strcmp(av[i], ";"))
            {
                i++;
                continue;
            }
            i += parse_argv(&tmp, &av[i]);
            
            if (!av[i])
                break;
            else
                i++;
        }
        if (tmp)
            exec_cmds(tmp, env);
        free_list(tmp);
    }
}
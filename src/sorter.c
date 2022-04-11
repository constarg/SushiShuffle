#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdio.h>

#include <sorter.h>

static struct config *config_file;
static pthread_mutex_t config_lock;



// the thread that will reparse the config file.
static void *refresh_config(void *arg)
{
	while (1)
	{
		return NULL;
		// sleep for the configured time.
		sleep(config_file->c_options.o_parse_interval);
		pthread_mutex_lock(&config_lock);
		// reparse the config. Thread safe.
		reparse_config(config_file);
		pthread_mutex_unlock(&config_lock);
	}
}

// get the last dot. That represents the extension of the file.
static inline char *extract_ext(const char *file_path)
{
	char *dot_loc = strstr(file_path, ".");
	char *tmp = dot_loc;
	while ((tmp = strstr(++tmp, ".")) != NULL)
	{
		dot_loc = tmp;
	}
	return dot_loc;
}

static inline int is_excluded(const char *path, const char *ext)
{
	char *(*exclude_l) = config_file->c_lists.l_exclude_list;
	for (int exl = 0; exclude_l[exl]; exl++)
	{ 
		if (
			strstr(exclude_l[exl], path) 
			&& 
			strstr(exclude_l[exl], ext)
 		   )
		{
			return 1;
		}
		else if (
			     strstr(exclude_l[exl], "*")
			     &&
			     strstr(exclude_l[exl], ext)
			)
		{
			return 1;
		}
	}
	return 0;
}

static inline char *get_target_rule(const char *ext)
{
	char *(*target_list) = config_file->c_lists.l_target_list;
	for (int tr = 0; target_list[tr]; tr++)
	{
		if (
			strstr(target_list[tr], ext)
			||
			strstr(target_list[tr], "*")
		   )
		{
			return strstr(target_list[tr], "/");
		}
	}
	return NULL;
}

static void move_file(const char *path, const char *file_name)
{
	char *ext = extract_ext(file_name);
	if (is_excluded(path, ext)) return;

	char *old_path = NULL;
	char *send_to = NULL;	
	char *target_path = get_target_rule(ext);

	old_path = (char *) malloc(sizeof(char) * (strlen(path) 
				   + strlen(file_name) + 1));
	if (old_path == NULL) return; // TODO: make a log.
	strcpy(old_path, path);
	strcat(old_path, file_name);

	if (!target_path)
	{
		char *default_path = config_file->c_options.o_default_path;
		send_to = (char *) malloc(sizeof(char) * (strlen(default_path)
					  + strlen(file_name) + 1));
		if (send_to == NULL) return; // TODO: make a log.
		strcpy(send_to, default_path);
		strcat(send_to, file_name);

		return;
	}
	send_to = (char *) malloc(sizeof(char) * (strlen(target_path)
				  + strlen(file_name) + 1));
	if (send_to == NULL) return; // TODO: make a log.
	strcpy(send_to, target_path);
	strcat(send_to, file_name);

	printf("send to: %s\n", send_to);
	// TODO: send to specified path.
}

static void search_files(const char *path)
{
	struct dirent *file = NULL;
	DIR *dir = NULL;
	// open the directory that represent by the path.
	dir = opendir(path);
	if (dir == NULL) return; // TODO: make a log here.

	while ((file = readdir(dir)) != NULL)
	{
		// if the current element of the directory is a file.
		if (file->d_type == DT_REG && file->d_name[0] != '.')
		{	
			move_file(path, file->d_name);
		}
	}
	closedir(dir);
}

// the thread that will organize the files.
static void *organize_files(void *arg)
{
	while (1)
	{
		// sleep for the configured time.
		//sleep(config_file->c_options.o_check_interval);
		pthread_mutex_lock(&config_lock);
		// read check list.
	
		for (int i = 0; config_file->c_lists.l_check_list[i]; i++)
		{
			search_files(config_file->c_lists.l_check_list[i]);
		}

		break;
	
		pthread_mutex_unlock(&config_lock);
	}
}



void start_sorter(struct config *src)
{
	config_file = src;
	// initalize the mutex.
	pthread_mutex_init(&config_lock, NULL);

	pthread_t refresh_t;
	pthread_t organize_t;
	// check if the threads has been created.
	if (
		pthread_create(&refresh_t, NULL, &refresh_config, NULL) != 0
		||
		pthread_create(&organize_t, NULL, &organize_files, NULL) != 0
	   )
	{
		// TODO - Log that the threads can't be created.
		pthread_mutex_destroy(&config_lock);
		return;
	}

	pthread_join(refresh_t, NULL);
	pthread_join(organize_t, NULL);

	// destroy the mutex.	
	pthread_mutex_destroy(&config_lock);
}
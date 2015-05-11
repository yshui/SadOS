#include <stdio.h>
#include <string.h>
#include <vfs.h>

void add_fs()
{

}

//check if current path is the root of a file system
int get_fs_index(char *str)
{
    int i;
    for (i = 0;i < super_block_count;++i)
        if (!strcmp(str, super_blocks[i].file_system_name) )
            return i;
    return -1;
}

//get current subdir
struct dentry* get_dentry(struct dentry* cur_dentry, char *path_part)
{
    int i;
    struct dentry* tmp_dentry;

    //enumerate each subdir
    for (i = 0;cur_dentry -> d_subdirs[i] != NULL;++i)
    {
        tmp_dentry = cur_dentry -> d_subdirs[i];
        if (!strcmp(tmp_dentry -> d_iname, path_part) )
            return tmp_dentry;
    }
    return NULL;
}

//parsing the given path
//firstly determine the file system, then determine the path
struct dentry* parse_path(char* path_str)
{
    int i, j, path_len = strlen(path_str);
    char tmp_path[200] = {0}, cur_path_part[200] = {0};
    int fs_index, max_len_fs = -1;
    struct dentry *cur_dentry;
    if (path_str[path_len - 1] != '/')
        path_str[path_len++] = '/';

    //file system
    for (i = 0;i < path_len;++i)
    {
        tmp_path[i] = path_str[i];
        if ( (fs_index = get_fs_index(tmp_path) ) != -1)
            max_len_fs = i;
    }
    if (max_len_fs == -1)
        return NULL;
    cur_dentry = super_blocks[fs_index].s_root;
    
    for (i = max_len_fs + 1;i < path_len;++i)
        tmp_path[i - max_len_fs - 1] = path_str[i];
    tmp_path[i] = '\0';

    for (i = 0;i < path_len;++i)
    {
        if (tmp_path[i] == '/')
        {
            memset(cur_path_part, 0, sizeof(cur_path_part) );
            for (j = 0;j < i;++j)
                cur_path_part[j] = tmp_path[j];
            if ( (cur_dentry = get_dentry(cur_dentry, cur_path_part) ) == NULL)
                return NULL;
            //prev = j + 1;
        }
    }
    return cur_dentry;
}

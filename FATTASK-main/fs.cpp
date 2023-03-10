#include "fs.h"
#include "cstring"
FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    current_block = ROOT_BLOCK;
    cwd = "/";
    disk.read(ROOT_BLOCK, (uint8_t *)files);
    disk.read(FAT_BLOCK, (uint8_t *)fat);
    position = find_number_of_files();
}

FS::~FS()
{
}
int
FS::getRelativeType(int type, std::string destpath)
{
    switch (type)
    {
        case CP:
            return find_free_entry(destpath);
            break;
        case MV:
            return find_free_entry(destpath);
            break;
        case APPEND:
            return find_file(destpath);
        default:
        return -1;
            break;
    }
}
int
FS::find_relative(std::string &sourcepath, std::string &destpath, dir_entry &src_file,
 dir_entry (&dest_files)[MAX_NO_OF_FILES], int &entry_file, relative_type type)
{
    unsigned int dest_block;
    std::string original_path = cwd;
    int source_file_index;
    dir_entry source_files[MAX_NO_OF_FILES];
    if (destpath.find("/") != std::string::npos)
    {
        std::string tmp_source_path = find_dir(destpath, SEARCH_FILE);
        if (tmp_source_path == "\0")
        {
            std::cout << "The directory does not exist: " << destpath << std::endl;
            return -1;
        }

        destpath = tmp_source_path;
        memcpy(dest_files, &files, sizeof(files));
        dest_block = current_block;
        entry_file = getRelativeType(type, destpath);
        if (entry_file == -1)
        {
            std::cout << "The destination you are trying to copy the file to is full"

            << destpath << std::endl;
            return -1;
        }
        else if(entry_file == -2)
        {
            std::cout << "The destination you are trying to copy the file already exists"
            << destpath << std::endl;
            return -1;
        }
        cd(original_path);
    }
    else
    {
        memcpy(dest_files, &files, sizeof(files));
        dest_block = current_block;
        entry_file = getRelativeType(type, destpath);
        if (entry_file == -1)
        {
            std::cout << "The destination you are trying to copy the file to is full: "
            << destpath << std::endl;
            return -1;
        }
        else if(entry_file == -2)
        {
            std::cout << "The destination you are trying to copy the file already exists: "
            << destpath << std::endl;
            return -1;
        }
    }

    if (sourcepath.find("/") != std::string::npos)
    {
        std::string tmp_filepath = find_dir(sourcepath, SEARCH_FILE);
        if (tmp_filepath == "\0")
        {
            std::cout << "The directory does not exist: " << sourcepath << std::endl;
            return -1;
        }
        sourcepath = tmp_filepath;
        //memcpy(&source_files, &files, sizeof(files));
        if (destpath == "source")
        {
            destpath = sourcepath;
        }
        source_file_index = find_file(sourcepath);
        src_file = files[source_file_index];
        if (cwd != original_path)
        {
            cd(original_path);
        }
    }
    else
    {
        if (destpath == "source")
        {
            destpath = sourcepath;
        }

        //memcpy(&source_files, &files, sizeof(files));
        source_file_index = find_file(sourcepath);
        if(source_file_index == -1)
        {
          std::cout << "File: " << sourcepath << " does not exist" << std::endl;
          return -1;
        }
        src_file = files[source_file_index];
    }
    cwd = original_path;
    return dest_block;
}
int FS::find_number_of_files()
{
    int i = 0;
    while (files[i].file_name[0] != '\0')
    {
        i++;
    }
    return i;
}
int FS::find_free_index(int16_t index)
{
    while (index < BLOCK_SIZE / 2 && fat[index] != FAT_FREE)
    {
        index++;
    }
    if (fat[index] != FAT_FREE)
    {
        return -1;
    }
    return index;
}
int FS::find_free_entry(const std::string filepath)
{
    
    int index = -1;
    for (size_t i = 0; i < MAX_NO_OF_FILES; i++)
    {
        if (files[i].file_name == filepath)
        {
            return -2;
        }
        if (files[i].file_name[0] == '\0')
        {
            if (index == -1)
            {
                index = i;
            }
        }

    }

    return index;
}
std::string
FS::find_dir(std::string filepath, int type)
{

    std::string tmp_cwd = cwd;
    std::string delim = "/";
    size_t pos = 0;
    std::string substr;
    if (filepath[0] == '/' && filepath.size() > 1)
    {
        cd("/"); // start at root
        filepath = filepath.substr(1, filepath.size() - 1);
    }
    if (filepath[filepath.size() - 1] == '/')
    {
        filepath[filepath.size() - 1] = '\0';
    }
    std::string tmp_path = filepath;
    while ((pos = tmp_path.find(delim)) != std::string::npos)
    {
        substr = tmp_path.substr(0, pos);
        int result = cd(substr);
        if (result == -1)
        {
            cd(tmp_cwd);
            return "\0";
        }
        tmp_path.erase(0, pos + delim.length());
    }
    int tmp_find = find_file(tmp_path);
    if (tmp_find != -1)
    {
        if (files[tmp_find].type == TYPE_DIR){
            cd(tmp_path);
            return "source";
        }
        return tmp_path;
    }
    return tmp_path;
}
int FS::find_file(std::string filepath)
{
    for (size_t i = 0; i < MAX_NO_OF_FILES; i++)
    {
        if (strcmp(files[i].file_name, filepath.c_str()) == 0)
        {
            return i;
        }
    }
    return -1;
}

// formats the disk, i.e., creates an empty file system
int FS::format()
{
    std::cout << "FS::format()\n";
    current_block = ROOT_BLOCK;
    char empty_char[1] = "";
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;
    disk.write(FAT_BLOCK, (uint8_t *)empty_char);
    cwd = "/";
    position = 0;
    disk.write(ROOT_BLOCK, (uint8_t *)empty_char);
    disk.read(ROOT_BLOCK, (uint8_t *)files);
    for (size_t i = 2; i < BLOCK_SIZE / 2; i++)
    {
        disk.write(i, (uint8_t *)empty_char);
        fat[i] = FAT_FREE;
    }
    memset(files, 0, sizeof(files));
    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{

    std::string original_path = cwd;
    if (filepath.find("/") != std::string::npos)
    {

        std::string tmp_filepath = find_dir(filepath, SEARCH_FILE);
        if (tmp_filepath == "\0")
        {
            std::cout << "The directory does not exist: " << filepath << std::endl;
            return -1;
        }
        filepath = tmp_filepath;
    }

    uint16_t entry_index = 2;
    if (filepath.size() < 57)
    {
        entry_index = find_free_index(entry_index);
        int entry_file = find_free_entry(filepath);
        int file_exists = find_file(filepath);
        if (entry_file == -1 || entry_index == -1)
        {
            std::cout << "Could not create file Disk is full, remove files or format disk" << std::endl;
            return -1;
        }
        else if (file_exists != -1)
        {
            std::cout << "Cannot create file, file already exists" << std::endl;
            return -1;
        }
        else
        {
            std::string in_buffer = "";
            std::string empty_cmp = " ";
            dir_entry file;
            file.first_blk = entry_index;
            file.access_rights = (READ | WRITE);
            file.type = TYPE_FILE;
            std::strcpy(file.file_name, filepath.c_str());

            while (empty_cmp != "")
            {
                std::getline(std::cin, empty_cmp);
                if (empty_cmp != "") {
                  in_buffer.append(empty_cmp + "\n");
                }

            }
            file.size = in_buffer.size();
            uint16_t prev_index;
            if (file.size > BLOCK_SIZE)
            {
                uint32_t substr_delimiter = BLOCK_SIZE - 1;
                int tmp_size = file.size;
                uint16_t index_substr = 0;
                while (tmp_size > BLOCK_SIZE)
                {
                    disk.write(entry_index, (uint8_t *)in_buffer.substr(index_substr, substr_delimiter).c_str());
                    index_substr += BLOCK_SIZE - 1;
                    tmp_size -= BLOCK_SIZE -1;

                    substr_delimiter += BLOCK_SIZE - 10;
                    prev_index = entry_index;

                    entry_index = find_free_index(prev_index+1);
                    if (entry_index == -1) {
                      std::cout << "There is no space on the drive to write this file"
                      << std::endl;
                      return -1;
                    }
                    fat[prev_index] = entry_index;
                }

                entry_index = find_free_index(entry_index);
                fat[prev_index] = entry_index;
                if (entry_index == -1) {
                  std::cout << "There is no space on the drive to write this file"
                  << std::endl;
                  return -1;
                }
                disk.write(entry_index, (uint8_t*) in_buffer.substr(index_substr, in_buffer.size()-1).c_str());
                fat[entry_index] = FAT_EOF;
            }
            else
            {
                disk.write(entry_index, (uint8_t *)in_buffer.c_str());
            }
            position = entry_file;
            fat[entry_index] = FAT_EOF;
            disk.write(FAT_BLOCK, (uint8_t *)fat);
            files[position] = file;
            position++;
            disk.write(current_block, (uint8_t *)files);
        }
    }

    std::cout << "FS::create(" << filepath << ")\n";
    cd(original_path);
    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    size_t i = 0;
    int result;
    int file_exists = find_file(filepath);
    if (file_exists == -1)
    {
        std::cout << "The file you are trying to cat does not exist." << std::endl;
        return -1;
    }
    else
    {
        if (files[file_exists].access_rights < 4)
        {
            std::cout << "You do not have access to read this file" << std::endl;
            return -1;
        }

        if (files[file_exists].access_rights == READ || files[file_exists].access_rights == (READ | WRITE) || files[file_exists].access_rights == (READ | WRITE | EXECUTE))
        {
            uint16_t file_blk = files[file_exists].first_blk;
            char buff_array[BLOCK_SIZE] = {""};
            while (fat[file_blk] != FAT_EOF)
            {

                disk.read(file_blk, (uint8_t *)buff_array);
                file_blk = fat[file_blk];
                std::cout << buff_array << std::endl;
            }
            disk.read(file_blk, (uint8_t *)buff_array);
            std::cout << buff_array << std::endl;
        }
    }
    std::cout << "FS::cat(" << filepath << ")\n";
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "FS::ls()\n";
    std::cout << "name\ttype\taccessrights\tsize"
              << std::endl;
    std::string type = "file";
    std::string dir = "-";
    char access_rights[4] = {'-',
                             '-',
                             '-', '\0'};

    disk.read(current_block, (uint8_t *)files);
    position = find_number_of_files();

    for (size_t i = 0; i < MAX_NO_OF_FILES; i++)
    {
        if (files[i].file_name == "\0")
        {
            continue;
        }

        switch (files[i].access_rights)
        {
        case (READ):
            access_rights[0] = 'r';
            access_rights[1] = '-';
            access_rights[2] = '-';
            break;
        case (WRITE):
            access_rights[0] = '-';
            access_rights[1] = 'w';
            access_rights[2] = '-';
            break;
        case (EXECUTE):
            access_rights[0] = '-';
            access_rights[1] = '-';
            access_rights[2] = 'x';
            break;
        case (READ | WRITE):
            access_rights[0] = 'r';
            access_rights[1] = 'w';
            access_rights[2] = '-';
            break;
        case (WRITE | EXECUTE):
            access_rights[0] = '-';
            access_rights[1] = 'w';
            access_rights[2] = 'x';
            break;
        case (READ | EXECUTE):
            access_rights[0] = 'r';
            access_rights[1] = '-';
            access_rights[2] = 'x';
            break;
        case (READ | WRITE | EXECUTE):
            access_rights[0] = 'r';
            access_rights[1] = 'w';
            access_rights[2] = 'x';
            break;
        default:
            access_rights[0] = '-';
            access_rights[1] = '-';
            access_rights[2] = '-';
            break;
        }
        type = "file";
        
        
        if (files[i].file_name[0] != '\0')
        {
            if (files[i].type == TYPE_DIR)
            {
                type = "dir";
                std::cout << files[i].file_name << "\t"
                      << type << "\t" << access_rights
                      << "\t\t" << dir << std::endl;
            }
            else
            {
            std::cout << files[i].file_name << "\t"
                      << type << "\t" << access_rights
                      << "\t\t" << files[i].size << std::endl;
            }
        }
    }
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    uint16_t entry_index = 0;
    std::string original_path = cwd;
    uint8_t original_blk = current_block;
    int entry_file;
    int dest_block;
    dir_entry source_file;
    dir_entry dest_files[MAX_NO_OF_FILES];
    if (sourcepath == destpath)
    {
        std::cout << "You are trying to copy the file with the same name." << std::endl;
        return -1;
    }


    int check_find = find_relative(sourcepath, destpath, source_file, dest_files, entry_file, CP);
    if (check_find == -1)
    {
        return -1;
    }
    if (source_file.access_rights < READ) {

      std::cout << "you cannot copy this file, you do not have read permission"
      << std::endl;
      return -1;
    }
    if (source_file.type == TYPE_DIR) {

      std::cout << "you cannot copy a directory" << std::endl;
      return -1;
    }
    dest_block = check_find;
    entry_index = find_free_index(entry_index);

    if (entry_index == -1)
    {
        std::cout << "Could not copy file Disk is full, remove files or format disk" << std::endl;
        return -1;
    }
    else
    {
        char buff_array[BLOCK_SIZE] = {""};
        std::string copied_content;
        dir_entry copied_file;
        //dir_entry source_file = source_files[source_file_index];
        uint16_t src_file_block = source_file.first_blk;
        if (destpath.size() > 57)
        {
            std::cout << "File path/name too long" << std::endl;
            return -1;
        }
        std::strcpy(copied_file.file_name, destpath.c_str());
        copied_file.first_blk = entry_index;
        copied_file.access_rights = source_file.access_rights;
        copied_file.size = source_file.size;
        copied_file.type = copied_file.type;
        uint16_t prev_index;
        dest_files[entry_file] = copied_file;
        position = find_number_of_files();
        while (fat[src_file_block] != FAT_EOF)
        {
            disk.read(src_file_block, (uint8_t *)buff_array);
            copied_content += buff_array;
            src_file_block = fat[src_file_block];
        }
        disk.read(src_file_block, (uint8_t *)buff_array);
        copied_content += buff_array;
        if (copied_file.size > BLOCK_SIZE)
        {
            uint32_t substr_delimiter = BLOCK_SIZE - 1;
            uint32_t tmp_size = copied_file.size;
            uint16_t index_substr = 0;
            while (tmp_size > BLOCK_SIZE)
            {
                std::cout << "the index substr: " << index_substr << std::endl;
                disk.write(entry_index, (uint8_t *)copied_content.substr(index_substr, substr_delimiter).c_str());
                index_substr += BLOCK_SIZE - 1;
                prev_index = entry_index;
                entry_index = find_free_index(prev_index+1);
                if (entry_index == -1) {
                  std::cout << "there is no space to copy" << std::endl;
                  return -1;
                }
                tmp_size -= BLOCK_SIZE;
                substr_delimiter += BLOCK_SIZE-1;
                fat[prev_index] = entry_index;
            }
            entry_index = find_free_index(prev_index+1);
            if (entry_index == -1) {
              std::cout << "there is no space to copy" << std::endl;
              return -1;
            }
            fat[prev_index] = entry_index;
            std::cout << "the index substr: " << index_substr << std::endl;
            disk.write(entry_index, (uint8_t *)copied_content.substr(index_substr, copied_content.size()-1).c_str());
            fat[entry_index] = FAT_EOF;
        }
        else
        {
            disk.write(entry_index, (uint8_t *)copied_content.substr(0, BLOCK_SIZE - 1).c_str());
            fat[entry_index] = FAT_EOF;
        }
    }
    current_block = original_blk;
    cwd = original_path;
    disk.write(dest_block, (uint8_t *)dest_files);
    disk.write(FAT_BLOCK, (uint8_t *)fat);
    disk.read(current_block, (uint8_t *)files);
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    uint16_t tmp_block = current_block;
    std::string current_path = cwd;
    uint16_t tmp_src_block;
    int src_file_index;
    int dest_file_index;
    dir_entry src_files[MAX_NO_OF_FILES];
    dir_entry dest_file;
    size_t last_of_src = sourcepath.find_last_of("/");
    size_t last_of_dest = destpath.find_last_of("/");
    
    if (last_of_src != std::string::npos)
    {
        std::string src_file_name = sourcepath.substr(last_of_src+1, sourcepath.size()-1);
        if (cd(sourcepath) == -1)
        {
            return -1;
        }
        else
        {
            src_file_index = find_file(src_file_name);
            if (src_file_index == -1)
            {
                std::cout << "The file you are trying to move does not exist: " << src_file_name << std::endl;
                return -1;
            }
            else if (files[src_file_index].type == TYPE_DIR)
            {
                std::cout << "You cannot move a directory" << std::endl;
                return -1;
            }
            else if (files[src_file_index].access_rights < 4)
            {
                std::cout << "This file does not have access rights to move" << std::endl;
                return -1;
            }

            memcpy(&src_files, &files, sizeof(files));
            memcpy(&dest_file, &src_files[src_file_index], sizeof(dir_entry));
            memset(&src_files[src_file_index], 0, sizeof(dir_entry));
            tmp_src_block = current_block;
            disk.read(tmp_block, (uint8_t*)files);
            current_block = tmp_block;
            cwd = current_path;
            if (last_of_dest != std::string::npos)
            {
                std::string file_name = destpath.substr(last_of_src+1, sourcepath.size()-1);
                if (file_name.size() > 57)
                {
                    std::cout << "File path/name too long" << std::endl;
                    return -1;
                }
                
                if (cd(destpath) == -1)
                {
                    return -1;
                }
                size_t check_dir_relative = cwd.find_last_of("/");
                if (check_dir_relative != std::string::npos)
                {
                    std::string check_string = cwd.substr(check_dir_relative+1, cwd.size()-1);
                    if (file_name == check_string || file_name.size() == 0)
                    {
                        file_name = src_file_name;
                    }
                }

                dest_file_index = find_free_entry(file_name);
                if (find_file("..") != -1)
                {
                    uint8_t dir_rights = files[find_file("..")].access_rights;
                     if (dir_rights < 6)
                    {
                        std::cout << "You do not have access to write to this directory" << std::endl;
                        return -1;
                    }
                }




                if (dest_file_index == -1)
                {
                    std::cout << "The destination you are trying to move to is full" << std::endl;
                    return -1;
                }
                else if (dest_file_index == -2)
                {
                    std::cout << "The file with the name" << destpath << "Already exists" << std::endl;
                    return -1;
                }
                std::strcpy(dest_file.file_name, file_name.c_str());
                files[dest_file_index] = dest_file;
                disk.write(tmp_src_block, (uint8_t*)src_files);
                disk.write(current_block, (uint8_t*)files);
                disk.read(tmp_src_block, (uint8_t*)files);
                current_block = tmp_src_block;
                cwd = current_path;
                return 0;
            }
            else
            {
                if (destpath.size() > 57)
                {
                    std::cout << "File path/name too long" << std::endl;
                    return -1;
                }
                if (find_file("..") != -1)
                {
                    uint8_t dir_rights = files[find_file("..")].access_rights;
                     if (dir_rights < 6)
                    {
                        std::cout << "You do not have access to write to this directory" << std::endl;
                        return -1;
                    }
                }
                dest_file_index = find_free_entry(destpath);
                if (dest_file_index == -1)
                {
                    std::cout << "The destination you are trying to move to is full" << std::endl;
                    return -1;
                }
                else if (dest_file_index == -2)
                {
                    std::cout << "The file with the name" << destpath << "Already exists";
                    return -1;
                }
                std::strcpy(dest_file.file_name, destpath.c_str());
                files[dest_file_index] = dest_file;
                disk.write(tmp_src_block, (uint8_t*)src_files);
                disk.write(current_block, (uint8_t*)files);
                disk.read(tmp_src_block, (uint8_t*)files);
                current_block = tmp_src_block;
                cwd = current_path;
                return 0;
            }
        }
    }
    else
    {
        src_file_index = find_file(sourcepath);
        if (src_file_index == -1)
        {
            std::cout << "The file you are trying to move does not exist" << sourcepath << std::endl;
            return -1;
        }
        else if (files[src_file_index].type == TYPE_DIR)
        {
            std::cout << "You cannot move a directory" << std::endl;
            return -1;
        }
        tmp_src_block = current_block;
        memcpy(&src_files, &files, sizeof(files));
        memcpy(&dest_file, &src_files[src_file_index], sizeof(dir_entry));
        memset(&src_files[src_file_index], 0, sizeof(dir_entry));
        if (last_of_dest != std::string::npos)
        {
            std::string file_name = destpath.substr(last_of_dest+1, destpath.size()-1);
            if (file_name.size() > 57)
            {
                    std::cout << "File path/name too long" << std::endl;
                    return -1;
            }
            std::cout << "mv: " << file_name << std::endl;
            if (cd(destpath) == -1)
            {
                return -1;
            }
            size_t check_dir_relative = cwd.find_last_of("/");
            if (check_dir_relative != std::string::npos)
            {
                std::string check_string = cwd.substr(check_dir_relative+1, cwd.size()-1);
                if (file_name == check_string || file_name.size() == 0)
                {
                    file_name = sourcepath;
                }
            }
            if (find_file("..") != -1)
            {
                uint8_t dir_rights = files[find_file("..")].access_rights;
                    if (dir_rights < 6)
                {
                    std::cout << "You do not have access to write to this directory" << std::endl;
                    return -1;
                }
            }
            dest_file_index = find_free_entry(file_name);

            if (dest_file_index == -1)
            {
                std::cout << "The destination you are trying to move to is full" << std::endl;
                return -1;
            }
            else if (dest_file_index == -2)
            {
                std::cout << "The file with the name" << destpath << "Already exists";
                return -1;
            }
            std::strcpy(dest_file.file_name, file_name.c_str());
            files[dest_file_index] = dest_file;
            disk.write(tmp_src_block, (uint8_t*)src_files);
            disk.write(current_block, (uint8_t*)files);
            disk.read(tmp_src_block, (uint8_t*)files);
            cwd = current_path;
            current_block = tmp_src_block;
            return 0;
        }
        else
        {
            if (destpath.size() > 57)
            {
                std::cout << "File path/name too long" << std::endl;
                return -1;
            }
            if (find_file("..") != -1)
            {
                uint8_t dir_rights = files[find_file("..")].access_rights;
                    if (dir_rights < 6)
                {
                    std::cout << "You do not have access to write to this directory" << std::endl;
                    return -1;
                }
            }
            std::strcpy(dest_file.file_name, destpath.c_str());
            dest_file_index = find_free_entry(destpath);
            if (dest_file_index == -1)
            {
                src_files[src_file_index] = dest_file;
                disk.write(tmp_src_block, (uint8_t*)src_files);
                return 0;
            }
            else if (dest_file_index == -2)
            {
                std::cout << "The file with the name" << destpath << "Already exists" << std::endl;
                return -1;
            }
            std::strcpy(dest_file.file_name, destpath.c_str());
            src_files[dest_file_index] = dest_file;
            disk.write(current_block, (uint8_t*)src_files);
            disk.read(tmp_src_block, (uint8_t*)files);
            cwd = current_path;
            current_block = tmp_src_block;
            return 0;
        }
    }


    disk.read(tmp_src_block, (uint8_t*)files);
    cwd = current_path;
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath)
{
    std::string original_path = cwd;
    
    if (filepath.find("/") != std::string::npos)
    {

        std::string tmp_filepath = find_dir(filepath, SEARCH_FILE);
        if (tmp_filepath == "\0")
        {
            std::cout << "The directory does not exist: " << filepath << std::endl;
            return -1;
        }
        filepath = tmp_filepath;
    }
    int result = find_file(filepath);

    char empty_char[1] = "";

    if (result == -1)
    {
        std::cout << "Could not remove file: " << filepath << ", file does not exists" << std::endl;
    }
    else
    {
        if (files[result].type == TYPE_DIR)
        {
            std::cout << "You cannot use rm on a directory" << filepath << std::endl;
            return -1;
        }

        int block_to_rm = files[result].first_blk;
        int tmp_block;
        while (1)
        {
            if (fat[block_to_rm] != FAT_EOF)
            {
                tmp_block = block_to_rm;
                disk.write(block_to_rm, (uint8_t *)empty_char);
                block_to_rm = fat[block_to_rm];
                fat[tmp_block] = FAT_FREE;
            }
            if (fat[block_to_rm] == FAT_EOF)
            {
                disk.write(block_to_rm, (uint8_t *)empty_char);
                fat[block_to_rm] = FAT_FREE;
                break;
            }
        }
        memset(&files[result], 0, sizeof(dir_entry));
        disk.write(FAT_BLOCK, (uint8_t *)fat);
        disk.write(current_block, (uint8_t *)files); // nästa directory när hieracaial dir skapas
        position = find_number_of_files();
    }
    std::cout << "FS::rm(" << filepath << ")\n";
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2)
{
    dir_entry source_file;
    dir_entry dest_files[MAX_NO_OF_FILES];
    int entry_file;
    int dest_block;
    int check_find = find_relative(filepath1, filepath2, source_file, dest_files, entry_file, APPEND);
    if (check_find == -1)
    {
        return -1;
    }
    uint16_t check_src_file_rights = source_file.access_rights;
    uint16_t check_dest_file_rights = dest_files[entry_file].access_rights;
    dest_block = check_find;
    if (source_file.type == TYPE_DIR)
    {
        std::cout << "You cannot append a directory: " << source_file.file_name << std::endl;
        return -1;
    }

    if (check_src_file_rights < 4)
    {
        std::cout << "File " << filepath1 << " does not have access rights to append " << std::endl;
        return -1;
    }
    if (check_dest_file_rights < 6)
    {
        std::cout << "File " << filepath2 << " does not have access rights to append " << std::endl;
        return -1;
    }

    else
    {
        char buff_array[BLOCK_SIZE] = {""};
        char last_content_of_file[BLOCK_SIZE] = {""};
        int substr_delimitor = BLOCK_SIZE -1;
        int substr_start = 0;
        std::string last_content_str;
        std::string copied_content;
        std::string temp_content;
        int copied_content_size;
        uint16_t src_file_block = source_file.first_blk;
        uint16_t dest_file_block = dest_files[entry_file].first_blk;
        dir_entry* dest_file = &dest_files[entry_file];
        dir_entry tmp_dest = dest_files[entry_file];

        uint16_t last_block_dest = dest_file_block;
        if (dest_file->type == TYPE_DIR)
        {
            std::cout << "You cannot append a directory: " << dest_file->file_name << std::endl;
            return -1;
        }


        while(fat[src_file_block] != FAT_EOF)
        {
            disk.read(src_file_block, (uint8_t*)buff_array);
            copied_content += buff_array;
            src_file_block = fat[src_file_block];
        }
        disk.read(src_file_block, (uint8_t*)buff_array);
        copied_content += buff_array;

        while(fat[dest_file_block] != FAT_EOF)
        {
            dest_file_block = fat[dest_file_block];
        }

        copied_content_size = copied_content.length();
        last_block_dest = dest_file_block;
        disk.read(last_block_dest, (uint8_t*) last_content_of_file);
        last_content_str = last_content_of_file;
        int last_content_size = last_content_str.size();
        if (last_content_size < BLOCK_SIZE -1)
        {
            int subtr_end = substr_delimitor - last_content_size;
            temp_content = copied_content.substr(substr_start, subtr_end);
            last_content_str += temp_content;
            substr_start += temp_content.size();
            dest_file->size = last_content_str.size();
            copied_content_size -= subtr_end;
            disk.write(last_block_dest, (uint8_t*) last_content_str.c_str());
        }
        if (copied_content_size > 0)
        {
            int prev_file_block = dest_file_block;
            dest_file_block = find_free_index(dest_file_block);
            if (dest_file_block == -1)
            {
                return 0;
            }
            else
            {
                fat[prev_file_block] = dest_file_block;
                while (copied_content_size > 0)
                {
                    if (copied_content_size > BLOCK_SIZE)
                    {
                        disk.write(dest_file_block, (uint8_t*)
                        copied_content.substr(substr_start, substr_delimitor).c_str());

                        substr_start += BLOCK_SIZE;
                        substr_delimitor += BLOCK_SIZE;
                        copied_content_size -= BLOCK_SIZE;
                        prev_file_block = dest_file_block;
                        dest_file_block = find_free_index(prev_file_block+1);
                        if (dest_file_block == -1)
                        {
                            std::cout << "There is no more space on the drive to copy more data" << std::endl;
                            return 0;
                        }
                        fat[prev_file_block] = dest_file_block;
                    }
                    else{
                    disk.write(dest_file_block, (uint8_t*)copied_content.substr(substr_start, copied_content.size()-1).c_str());
                    break;
                  }

                }
            }
    }

    int final_size = dest_file->size + copied_content.size();
    dest_file->size = tmp_dest.size + copied_content.size();
    fat[dest_file_block] = FAT_EOF;
    disk.write(FAT_BLOCK, (uint8_t*) fat);
    disk.write(dest_block, (uint8_t*)dest_files);


    }
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath)
{

    std::string original_dir = cwd;
    if (dirpath.find("/") != std::string::npos)
    {

        std::string tmp_filepath = find_dir(dirpath, SEARCH_FILE);
        if (tmp_filepath == "\0")
        {
            std::cout << "The directory does not exist: " << dirpath << std::endl;
            return -1;
        }
        dirpath = tmp_filepath;
    }
    int entry_dir = find_free_entry(dirpath);
    int entry_index = find_free_index(entry_dir);
    int check_name_dir = find_file(dirpath);
    if (entry_dir == -1 || entry_index == -1)
    {
        std::cout << "Disk full" << std::endl;
        return -1;
    }
    if (check_name_dir != -1)
    {
        std::cout << "\nDirectory already exists" << std::endl;
        return -1;
    }
    else
    {
        if (dirpath.size() > 57)
        {
         std::cout << "File path/name too long" << std::endl;
         return -1;
        }
        dir_entry entry_files[MAX_NO_OF_FILES];
        dir_entry new_dir;
        std::strcpy(new_dir.file_name, dirpath.c_str());
        new_dir.type = TYPE_DIR;
        new_dir.first_blk = entry_index;
        new_dir.size = 1;
        new_dir.access_rights = (READ | WRITE);
        files[entry_dir] = new_dir;

        memset(entry_files, 0, sizeof(entry_files));
        strcpy(entry_files[0].file_name, "..");
        entry_files[0].first_blk = current_block;
        entry_files[0].type = TYPE_DIR;
        entry_files[0].access_rights = (READ | WRITE);  
        
        disk.write(entry_index, (uint8_t *)entry_files);
        fat[entry_index] = FAT_EOF;
        disk.write(FAT_BLOCK, (uint8_t *)fat);
        disk.write(current_block, (uint8_t *)files);
        current_block = entry_index;
    }
    cd(original_dir);
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    int check_dir = find_file(dirpath);
    dir_entry dir = files[check_dir];

    if (dirpath.find("/") != std::string::npos && dirpath.size() > 1)
    {
        std::string dir = find_dir(dirpath, SERACH_FOLDER);
        if (dir == "\0")
        {
            std::cout << "The directory you are trying to cd does not exist " << dirpath << std::endl;
            return -1;
        }
        return 0;
    }

    if (dirpath == "/")
    {
        cwd = "/";
        disk.read(ROOT_BLOCK, (uint8_t *)files);
        current_block = ROOT_BLOCK;
        return 0;
    }
    if (cwd != "/" && dirpath != "..")
    {
        cwd += "/";
    }

    if (check_dir == -1)
    {
        std::cout << dirpath << std::endl;
        std::cout << "Directory does not exist" << std::endl;
        return -1;
    }
    else
    {
        if (dir.access_rights < 4)
        {
          std::cout << "you do not have access rights to read this directory: " << dir.file_name
          << std::endl;
          return -1;
        }
        if (dir.type == TYPE_FILE)
        {
            std::cout << "You are trying to enter a file: " << dirpath << std::endl;
            return -1;
        }

        disk.read(dir.first_blk, (uint8_t *)files);
        current_block = dir.first_blk;
        size_t found = cwd.find_last_of("/");
        if (current_block == ROOT_BLOCK)
        {
            cwd = "/";
        }
        else if (dirpath == "..")
        {
            if (found != std::string::npos)
            {
                cwd = cwd.substr(0, found);
            }
        }
        else
        {
            cwd += dirpath;
        }
    }
    position = find_number_of_files();

    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int FS::pwd()
{
    std::cout << "FS::pwd()\n";
    std::cout << cwd << std::endl;
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath)
{
    int rights;
    int file_to_change = find_file(filepath);

    try
    {
        rights = stoi(accessrights);
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Argument is not an integer" << std::endl;
        return -1;
    }

    if (file_to_change == -1)
    {
        std::cout << "No such file or directory" << std::endl;
        return -1;
    }

    else

        if (rights >= 0 && rights <= (READ | WRITE | EXECUTE))
    {
        files[file_to_change].access_rights = rights;
        disk.write(current_block, (uint8_t *)files);
    }
    else
    {
        std::cout << "Argument can only range between 0-7" << std::endl;
        return -1;
    }

    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}

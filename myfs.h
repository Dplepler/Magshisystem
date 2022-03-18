#ifndef __MYFS_H__
#define __MYFS_H__

#define DATA_OFFSET 2048 	 	// This is where the data starts in our block device

#define END_OF_FILE "0xFFFF" 	// End of file signature

#define MAX_FILENAME_LENGTH 10

#define BLOCK_DEVICE_SIZE 1024 * 1024
#define BLOCK_SIZE 256

#define BLOCK_INFO_LENGTH 3 	// Offset of data block that is reserved for params: Occupied(1 byte), next block(2 bytes)

#define CONTENT_LENGTH BLOCK_SIZE - BLOCK_INFO_LENGTH

#include <memory>
#include <vector>
#include <stdint.h>
#include "blkdev.h"


class MyFs {
public:
	MyFs(BlockDeviceSimulator *blkdevsim_);


	typedef struct FOLDER_NODE_STRUCT {

		char filename[MAX_FILENAME_LENGTH + 1];
		char type;
		unsigned int position;
	} inode_t;

	typedef struct FOLDER_STRUCT {
		std::vector<inode_t> file_entries;
		
	} folder_t;

	/**
	 * format method
	 * This function discards the current content in the blockdevice and
	 * create a fresh new MYFS instance in the blockdevice.
	 */
	void format();

	/**
	 * create_file method
	 * Creates a new file in the required path.
	 * @param path_str the file path (e.g. "/newfile")
	 * @param directory boolean indicating whether this is a file or directory
	 */
	void create_file(std::string path_str, bool directory);

	/**
	 * get_content method
	 * Returns the whole content of the file indicated by path_str param.
	 * Note: this method assumes path_str refers to a file and not a
	 * directory.
	 * @param path_str the file path (e.g. "/somefile")
	 * @return the content of the file
	 */
	std::string get_content(std::string path_str);

	/**
	 * set_content method
	 * Sets the whole content of the file indicated by path_str param.
	 * Note: this method assumes path_str refers to a file and not a
	 * directory.
	 * @param path_str the file path (e.g. "/somefile")
	 * @param content the file content string
	 */
	void set_content(std::string path_str, std::string content);

	/**
	 * list_dir method
	 * Returns a list of a files in a directory.
	 * Note: this method assumes path_str refers to a directory and not a
	 * file.
	 * @param path_str the file path (e.g. "/somedir")
	 * @return a vector of dir_list_entry structures, one for each file in
	 *	the directory.
	 */
	folder_t list_dir(std::string path_str);

	bool file_already_exists(const char* filename);
	inode_t get_file_inode(const char* path_str);
	void reset_contents(unsigned int position);
	unsigned int find_free_block();
	void write_folder(folder_t folder);
	folder_t get_folder();


private:

	/**
	 * This struct represents the first bytes of a myfs filesystem.
	 * It holds some magic characters and a number indicating the version.
	 * Upon class construction, the magic and the header are tested - if
	 * they both exist than the file is assumed to contain a valid myfs
	 * instance. Otherwise, the blockdevice is formated and a new instance is
	 * created.
	 */
	struct myfs_header {
		char magic[4];
		uint8_t version;
	};



	BlockDeviceSimulator *blkdevsim;

	static const uint8_t CURR_VERSION = 0x03;
	static const char *MYFS_MAGIC;
};

#endif // __MYFS_H__

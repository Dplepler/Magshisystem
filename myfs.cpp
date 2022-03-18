#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>

const char *MyFs::MYFS_MAGIC = "MYFS";

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_) {
	struct myfs_header header;
	blkdevsim->read(0, sizeof(header), (char *)&header);

	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) {
		std::cout << "Did not find myfs instance on blkdev" << std::endl;
		std::cout << "Creating..." << std::endl;
		format();
		std::cout << "Finished!" << std::endl;
	}
}

void MyFs::format() {

	// put the header in place
	struct myfs_header header;
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(header), (const char*)&header);

	// TODO: put your format code here
}

void MyFs::create_file(std::string path_str, bool directory) {
	
	unsigned int position = 0;
	char buffer[2];

	if (file_already_exists(path_str.c_str())) {
		throw std::runtime_error("Filename already exists");
	}

	/* Iterate through all the data available, and find an empty block to start the file from */
	for (unsigned int i = 0; i < BLOCK_DEVICE_SIZE - DATA_OFFSET; i += BLOCK_SIZE) {

		this->blkdevsim->read(i, sizeof(bool), buffer);
		buffer[1] = '\0';

		// Check if an empty block was found - if so, break out of the loop and use it
		if (!strcmp(buffer, "0")) { position = i; break; }
	}

	// If we couldn't find even 1 free block, exit
	if (!position) {
		throw std::runtime_error("Not enough disk space!");
	}

	inode_t new_file_entry;

	this->blkdevsim->write(position, sizeof(bool), "1"); 	// Make sure to mark the block as occupied 

	/* Set inode parameters */
	strcpy(new_file_entry.filename, path_str.c_str()); 				// Copy file name to the new node
	new_file_entry.type = directory ? 'D' : 'F'; 								
	new_file_entry.position = position;

	/* Add file to the root directory */
	folder_t root;
	this->blkdevsim->read(sizeof(struct myfs_header), sizeof(folder_t), (char*)&root); 		// The root directory starts after the file header

	root.file_entries.push_back(new_file_entry); 											// Push data onto root directory buffer

	this->blkdevsim->write(sizeof(struct myfs_header), sizeof(folder_t), (char*)&root); 	// Write data back to the root directory
}

std::string MyFs::get_content(std::string path_str) {
	throw std::runtime_error("not implemented");
	return "";
}

void MyFs::set_content(std::string path_str, std::string content) {
	throw std::runtime_error("not implemented");
}

MyFs::dir_list MyFs::list_dir(std::string path_str) {
	dir_list ans;
	throw std::runtime_error("not implemented");
	return ans;
}

/*
file_already_exists checks if a filename already exists in the current block device
Input: Filename to search for
Output: True if filename was found, otherwise false
*/
bool MyFs::file_already_exists(const char* filename) {

	inode_t inode;

	for (unsigned int i = 0; i < DATA_OFFSET; i += sizeof(inode_t)) {

		this->blkdevsim->read(i, sizeof(inode), (char*)&inode);
		if (!strcmp(inode.filename, filename)) {
			return true;
		}
	}

	return false;
}


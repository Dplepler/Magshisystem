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

	folder_t folder;
	this->blkdevsim->write(sizeof(myfs_header), sizeof(folder_t), (const char*)&folder);
}

void MyFs::create_file(std::string path_str, bool directory) {

	if (file_already_exists(path_str.c_str())) { throw std::runtime_error("Filename already exists"); }

	unsigned int position = find_free_block();

	inode_t new_file_entry;

	this->blkdevsim->write(position, sizeof(bool), "1"); 				// Make sure to mark the block as occupied 
	this->blkdevsim->write(position + 1, sizeof(short), END_OF_FILE); 	// Specify that the new block is the current end of the file

	/* Set inode parameters */
	strcpy(new_file_entry.filename, path_str.c_str()); 					// Copy file name to the new node
	new_file_entry.type = directory ? 'D' : 'F'; 								
	new_file_entry.position = position;
	std:: cout << new_file_entry.filename << std::endl;

	/* Add file to the root directory */
	folder_t root = get_folder();
	
	root.file_entries.push_back(new_file_entry);		// Push data onto root directory buffer

	write_folder(root); 								// Write data back to the root directory
}

std::string MyFs::get_content(std::string path_str) {
	
	std::string contents = "";

	char buffer[BLOCK_SIZE + 1] = { 0 };

	inode_t inode = get_file_inode(path_str.c_str());

	std::string block;
	unsigned int position = inode.position;

	/* Go through all the file's blocks and concatenate their contents into one string */
	for (unsigned int i = DATA_OFFSET; i < (BLOCK_DEVICE_SIZE / BLOCK_SIZE) - DATA_OFFSET; i++) {

		this->blkdevsim->read(position, BLOCK_SIZE, buffer);
		buffer[BLOCK_SIZE] = '\0';

		block = std::string(buffer);

		std::cout << block << std::endl;

		contents += std::string(block.substr(BLOCK_INFO_LENGTH + 1)); 	// Concatenate file contents

		/* exit loop if we reached EOF */
		if (block.substr(sizeof(bool), BLOCK_INFO_LENGTH - 1) == END_OF_FILE) { 	
			break;
		} 
		
		position = std::stoul(block.substr(sizeof(bool), BLOCK_INFO_LENGTH - 1));
	} 

	return contents;
}

void MyFs::set_content(std::string path_str, std::string content) {
	
	inode_t inode = get_file_inode(path_str.c_str());
	std::string block;
	
	this->reset_contents(inode.position); 	// Reset and clear all data

	unsigned int position = inode.position;
	unsigned int new_pos = 0;

	while (content.length() > 0) {

		this->blkdevsim->write(position + BLOCK_INFO_LENGTH, 
			content.length() > CONTENT_LENGTH ? 
			CONTENT_LENGTH : content.length(), content.substr(0, CONTENT_LENGTH).c_str());

		// Take out all the content we already added, if this was the last bit of text, we can make content equal nothing
		content = content.length() > CONTENT_LENGTH ? content.substr(CONTENT_LENGTH, content.length()) : ""; 	

		new_pos = find_free_block();

		this->blkdevsim->write(position, sizeof(short), std::to_string(new_pos).c_str());
	}
}

MyFs::folder_t MyFs::list_dir(std::string path_str) {
	
	return get_folder();
}

/*
file_already_exists checks if a filename already exists in the current block device
Input: Filename to search for
Output: True if filename was found, otherwise false
*/
bool MyFs::file_already_exists(const char* filename) {

	folder_t folder = get_folder();

	size_t length = folder.file_entries.size();

	std::cout << length;

	/* Find file inode */
	for (unsigned int i = 0; i < length; i++) {
		if (!strcmp(folder.file_entries[i].filename, filename)) { return true; }
	}

	
	return false;
}

MyFs::inode_t MyFs::get_file_inode(const char* path_str) {

	folder_t folder = get_folder();
	
	size_t length = folder.file_entries.size();


	/* Find file inode */
	for (unsigned int i = 0; i < length; i++) {
		if (!strcmp(folder.file_entries[i].filename, path_str)) { return folder.file_entries[i]; }
	}

	// If the file was not found..
	throw std::runtime_error("File does not exist");
}

void MyFs::reset_contents(unsigned int position) {

	char buffer[sizeof(short) + 1];

	/* Go through all the file's blocks and reset their content */
	for (unsigned int i = DATA_OFFSET; i < (BLOCK_DEVICE_SIZE / BLOCK_SIZE) - DATA_OFFSET; i++) {

		this->blkdevsim->write(position, sizeof(bool), "0");

		this->blkdevsim->read(position + 1, sizeof(short), buffer);
		buffer[sizeof(short)] = '\0';

		/* exit loop if we reached EOF */
		if (!strcmp(buffer, END_OF_FILE)) {
			break;
		} 
		
		position = std::stoul(buffer);
	} 
}

unsigned int MyFs::find_free_block() {

	unsigned int position = 0;
	char buffer[2];

	/* Iterate through all the data available, and find an empty block */
	for (unsigned int i = DATA_OFFSET; i < BLOCK_DEVICE_SIZE - DATA_OFFSET; i += BLOCK_SIZE) {

		this->blkdevsim->read(i, 1, buffer);
		buffer[1] = '\0';

		std::cout << buffer << std::endl;

		// Check if an empty block was found - if so, break out of the loop and use it
		if (!buffer[0]) { position = i; break; }
	}

	// If we couldn't find even 1 free block, exit
	if (!position) { throw std::runtime_error("Not enough disk space!"); }

	return position;
}

void MyFs::write_folder(folder_t folder) {

	size_t length = folder.file_entries.size();

	unsigned int position = sizeof(myfs_header); 	// Start position

	for (unsigned int i = 0; i < length; i++, position += sizeof(inode_t)) {

		this->blkdevsim->write(position, sizeof(inode_t), (char*)&folder.file_entries[i]);
		
	}

}

MyFs::folder_t MyFs::get_folder() {

	folder_t folder;
	inode_t inode;
	char buffer [1];

	unsigned int position = sizeof(myfs_header);

	for (unsigned int i = 0; i < DATA_OFFSET; i++, position += sizeof(inode_t)) {

		this->blkdevsim->read(position, 1, buffer);
		if (!buffer[0]) { break; }

		this->blkdevsim->read(position, sizeof(inode_t), (char*)&inode);

		folder.file_entries.push_back(inode);
	}

	return folder;
}


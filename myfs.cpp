#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>

const char *MyFs::MYFS_MAGIC = "MYFS";
static const unsigned short END_OF_FILE = 0xFFFF; 		// End of file signature

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

/*
create_file creates a new file with a specified path
Input: New file's path, a flag to decide if file will be a directory or not
Output: None
*/
void MyFs::create_file(std::string path_str, bool directory) {

	if (file_already_exists(path_str.c_str())) { throw std::runtime_error("Filename already exists"); }

	unsigned int position = find_free_block();

	inode_t new_file_entry;

	this->blkdevsim->write(position, sizeof(bool), "\1"); 						// Make sure to mark the block as occupied 
	this->blkdevsim->write(position + 1, sizeof(short), (char*)&END_OF_FILE); 	// Specify that the new block is the current end of the file

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

/*
get_content returns the data of a specified file
Input: File path
Output: Content of requested file
*/
std::string MyFs::get_content(std::string path_str) {
	
	std::string contents = "";

	char buffer[BLOCK_SIZE - BLOCK_INFO_LENGTH + 1] = { 0 };
	unsigned short index_buffer;

	inode_t inode = get_file_inode(path_str.c_str());

	std::string block;
	unsigned int position = inode.position;

	/* Go through all the file's blocks and concatenate their contents into one string */
	for (unsigned int i = 0; i < (BLOCK_DEVICE_SIZE - DATA_OFFSET) / BLOCK_SIZE; i++) {

		this->blkdevsim->read(position + BLOCK_INFO_LENGTH, BLOCK_SIZE, buffer);
		buffer[BLOCK_SIZE - BLOCK_INFO_LENGTH] = '\0';

		block = std::string(buffer);

		contents += block; 			// Concatenate file contents

		/* exit loop if we reached EOF */
		this->blkdevsim->read(position + 1, sizeof(short), (char*)&index_buffer);

		if (index_buffer == END_OF_FILE) { 
			break;
		} 
		
		position = index_buffer;
	} 

	return contents;
}

/*
set_content assigns new given data to a specified file
Input: File path, data to assign
Output: None

NOTE: OLD DATA WILL BE DELETED AND REPLACED

*/
void MyFs::set_content(std::string path_str, std::string content) {
	
	inode_t inode = this->get_file_inode(path_str.c_str());
	std::string block;
	bool clear = false;

	this->set_size(path_str.c_str(), content.length());
	
	this->reset_contents(inode.position); 	// Reset and clear all data

	unsigned int position = inode.position;
	unsigned int new_pos = 0;

	while (content.length() > 0) {

		clear = CONTENT_LENGTH > content.length();

		if (clear) { this->clear_block(position); }

		this->blkdevsim->write(position, sizeof(bool), "\1"); 

		this->blkdevsim->write(position + BLOCK_INFO_LENGTH, 
			!clear ? CONTENT_LENGTH : content.length(),
			!clear ? content.substr(0, CONTENT_LENGTH).c_str() : content.c_str());

		// Take out all the content we already added, if this was the last bit of text, we can make content equal nothing
		content = content.length() > CONTENT_LENGTH ? content.substr(CONTENT_LENGTH) : ""; 	

		new_pos = find_free_block();
		
		this->blkdevsim->write(position + 1, sizeof(short), content == "" ? (char*)&END_OF_FILE : (char*)&new_pos);

		position = new_pos;
	}
}

/*
list_dir returns all the files in the current folder
Input: I have no idea why we need path_str 
Output: Folder that contains all the paths
*/
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

	/* Find file inode */
	for (unsigned int i = 0; i < length; i++) {
		if (!strcmp(folder.file_entries[i].filename, filename)) { return true; }
	}

	
	return false;
}

/*
get_file_inode returns the inode that represents a given filename
Input: File path to get the inode of
Output: The inode of the requested file
*/
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

/*
reset_contents clears all the data of a specified file
Input: The position of the first block in memory
Output: None
*/
void MyFs::reset_contents(unsigned int position) {

	unsigned short buffer;

	/* Go through all the file's blocks and reset their content */
	for (unsigned int i = DATA_OFFSET; i < (BLOCK_DEVICE_SIZE / BLOCK_SIZE) - DATA_OFFSET; i++) {

		this->blkdevsim->write(position, sizeof(bool), 0);

		this->blkdevsim->read(position + 1, sizeof(short), (char*)&buffer);

		/* exit loop if we reached EOF */
		if (buffer == END_OF_FILE) {
			break;
		} 
		
		position = buffer;
	} 
}

/*
find_free_block returns an unused block of data
Input: None
Output: The position of the free block, error if there's not enough disk space
*/
unsigned int MyFs::find_free_block() {

	unsigned int position = 0;
	char buffer;

	/* Iterate through all the data available, and find an empty block */
	for (unsigned int i = DATA_OFFSET; i < BLOCK_DEVICE_SIZE - DATA_OFFSET; i += BLOCK_SIZE) {

		this->blkdevsim->read(i, sizeof(bool), (char*)&buffer);

		// Check if an empty block was found - if so, break out of the loop and use it
		if (buffer == '\0') { position = i; break; }

	}

	// If we couldn't find even 1 free block, exit
	if (!position) { throw std::runtime_error("Not enough disk space!"); }

	return position;
}

/*
write_folder writes a given folder and replaces the root with it
Input: New folder to write to the root directory memory
Output: None
*/
void MyFs::write_folder(folder_t folder) {

	size_t length = folder.file_entries.size();

	unsigned int position = sizeof(myfs_header); 	// Start position

	for (unsigned int i = 0; i < length; i++, position += sizeof(inode_t)) {

		this->blkdevsim->write(position, sizeof(inode_t), (char*)&folder.file_entries[i]);
		
	}

}

/*
get_folder get's all the inodes at the root directory into a folder and returns it
Input: None
Output: The contents of the root directory
*/
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


/*
clear_block clears the whole block and resets it's data to 0
Input: Start of block position
Output: None
*/
void MyFs::clear_block(unsigned int position) {

	char data[BLOCK_SIZE] = { '\0' };
	this->blkdevsim->write(position, BLOCK_SIZE, data);
}


/*
set_size replaces an inode's size with a new one
Input: Inode's filename, new size
Output: None
*/
void MyFs::set_size(const char* filename, size_t new_size) {

	folder_t folder = get_folder();
	size_t length = folder.file_entries.size();

	/* Find the inode and replace it's size */
	for (unsigned int i = 0; i < length; i++) {

		if (!strcmp(filename, folder.file_entries[i].filename)) {
			folder.file_entries[i].size = new_size;
		}
	}

	this->write_folder(folder);
}
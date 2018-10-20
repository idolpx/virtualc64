/*
 * (C) 2007 Dirk W. Hoffmann. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PRGFile.h"

PRGFile::PRGFile()
{
    setDescription("PRGArchive");
}

PRGFile *
PRGFile::makePRGArchiveWithBuffer(const uint8_t *buffer, size_t length)
{
    PRGFile *archive = new PRGFile();
    
    if (!archive->readFromBuffer(buffer, length)) {
        delete archive;
        return NULL;
    }
    
    return archive;
}

PRGFile *
PRGFile::makePRGArchiveWithFile(const char *filename)
{
    PRGFile *archive = new PRGFile();
    
    if (!archive->readFromFile(filename)) {
        delete archive;
        return NULL;
    }
    
    return archive;
}

PRGFile *
PRGFile::makePRGArchiveWithAnyArchive(Archive *otherArchive) {
    
    int exportItem = 0;
    
    if (otherArchive == NULL || otherArchive->getNumberOfItems() <= exportItem)
        return NULL;
    
    PRGFile *archive = new PRGFile();
    archive->debug(1, "Creating PRG archive from %s archive...\n",
                   otherArchive->typeAsString());
    
    // Determine container size and allocate memory
    archive->size = 2 + otherArchive->getSizeOfItem(exportItem);
    if ((archive->data = new uint8_t[archive->size]) == NULL) {
        archive->warn("Failed to allocate %d bytes of memory\n", archive->size);
        delete archive;
        return NULL;
    }
    
    // Load address
    uint8_t* ptr = archive->data;
    *ptr++ = LO_BYTE(otherArchive->getDestinationAddrOfItem(exportItem));
    *ptr++ = HI_BYTE(otherArchive->getDestinationAddrOfItem(exportItem));
    
    // File data
    int byte;
    otherArchive->selectItem(exportItem);
    while ((byte = otherArchive->getByte()) != EOF) {
        *ptr++ = (uint8_t)byte;
    }
    
    return archive;
}

bool
PRGFile::isPRG(const uint8_t *buffer, size_t length)
{
    return length >= 2;
}

bool 
PRGFile::isPRGFile(const char *filename)
{
	assert(filename != NULL);
	
	if (!checkFileSuffix(filename, ".PRG") && !checkFileSuffix(filename, ".prg"))
		return false;
	
	if (!checkFileSize(filename, 2, -1))
		return false;
	
	return true;
}

void 
PRGFile::dealloc()
{
	if (data) free(data);
	data = NULL;
	size = 0;
	fp = -1;
}

bool 
PRGFile::hasSameType(const char *filename)
{
	return isPRGFile(filename);
}

size_t
PRGFile::writeToBuffer(uint8_t *buffer)
{
    assert(data != NULL);

    if (buffer) {
        memcpy(buffer, data, size);
    }
    return size;
}

int
PRGFile::getNumberOfItems()
{
	return 1;
}

const char *
PRGFile::getNameOfItem(unsigned n)
{
    assert(n < getNumberOfItems());
	return "FILE";
}

const char *
PRGFile::getTypeOfItem(unsigned n)
{
	return "PRG";
}

uint16_t 
PRGFile::getDestinationAddrOfItem(unsigned n)
{
	uint16_t result = LO_HI(data[0], data[1]);
	return result;
}

void 
PRGFile::selectItem(unsigned n)
{
	fp = 2; // skip load address

	if (fp >= size)
		fp = -1;
}

int 
PRGFile::getByte()
{
	int result;
	
	if (fp < 0)
		return -1;
		
	// get byte
	result = data[fp++];
	
	// check for end of file
	if (fp == size)
		fp = -1;

	return result;
}

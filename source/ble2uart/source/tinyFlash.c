#include "tl_common.h"
#include "drivers.h"

#include "tinyFlash.h"

#define TINY_SECTOR_SIZE 4096  // flash sector size
#define TINY_BUFFER_SIZE 256  // tiny buffer size

#define TINY_SECHAD_SIZE 32  // Sector header size to record sector usage

static unsigned char _buf[TINY_BUFFER_SIZE] = { 0 };

static unsigned long tinyFlash_Start_Addr = 0;
static unsigned long tinyFlash_End_Addr   = 0;

static unsigned long tinyFlash_Used_Addr = 0;  // Currently used sector address
static unsigned long tinyFlash_Swap_Addr = 0;  // Currently unused sector address

void flash_write_org(unsigned long addr, unsigned long len, unsigned char *buf)
{
    unsigned long tmp = addr & 0xff;

    if(tmp + len > 0x100)  // Write across sectors
    {
        flash_write_page(addr, 0x100 - tmp, buf);

        tmp = (0x100 - tmp);

        len -= tmp;

        buf += tmp;

        addr &= 0xffffff00;

        addr += 0x100;
    }

    flash_write_page(addr, len, buf);
}

// flash_erase_sector(addr);
// flash_write_page(addr, 256, buf);
// flash_read_page(unsigned long addr, unsigned long len, unsigned char *buf){

bool tinyFlash_Init(unsigned long start_addr, unsigned long len)
{
    tinyFlash_Start_Addr = start_addr;

    tinyFlash_End_Addr = start_addr + len;

    memset(_buf, 0 , TINY_BUFFER_SIZE);

    flash_read_page(tinyFlash_Start_Addr, TINY_BUFFER_SIZE, _buf);

    if(_buf[0] != 0XFF)  // The first sector is in use
    {
        tinyFlash_Used_Addr = tinyFlash_Start_Addr;
        tinyFlash_Swap_Addr = tinyFlash_Start_Addr + TINY_SECTOR_SIZE;
    }
    else  // The second sector is in use
    {
        tinyFlash_Used_Addr = tinyFlash_Start_Addr + TINY_SECTOR_SIZE;
        tinyFlash_Swap_Addr = tinyFlash_Start_Addr;
    }

    return true;
}
		
int tinyFlash_Read(unsigned char KEY, unsigned char * outbuf, unsigned char * len)
{
    unsigned long _addr_start = tinyFlash_Used_Addr + TINY_SECHAD_SIZE;;
    unsigned long _addr_end   = tinyFlash_Used_Addr + TINY_SECTOR_SIZE;
    
    while(1)
    {
        if(_addr_start > _addr_end -3)  // The sector search is completed
        {
            break;
        }
        
        flash_read_page(_addr_start, TINY_BUFFER_SIZE, _buf);

        if(_buf[0] == KEY)  // Target KEY
        {
            if(_buf[1] == (KEY ^ 0xFF))  // The KEY is in use
            {
                if(len == NULL)  // Delete operation
                {
                    _buf[0] = 0;
                    flash_write_page(_addr_start + 1 , 1 , _buf);
                }
                else if(outbuf == NULL)  // Read length
                {
                    *len = _buf[2];
                }
                else  // Read data
                {
                    memcpy(outbuf, _buf + 3, _buf[2]);
                    *len = _buf[2];
                }

                return 0;
            }
            else  // The KEY has been deleted
            {
                _addr_start += (_buf[2] + 3);
                continue;
            }
        }
        else if((_buf[0] != 0) && (_buf[0] != 0xff))  // Other KEY
        {
            _addr_start += (_buf[2] + 3);
            continue;
        }
        else  // Read the unused area at the end of the sector
        {
            break;
        }
    }
    
    return -1;
}

unsigned long  g_addr = 0;

int tinyFlash_Write(unsigned char KEY, unsigned char * buf, unsigned char len)
{
    tinyFlash_Read(KEY,  NULL, NULL);

    unsigned long _addr_start = tinyFlash_Used_Addr + TINY_SECHAD_SIZE;;
    unsigned long _addr_end   = tinyFlash_Used_Addr + TINY_SECTOR_SIZE;

    unsigned long dirty_data_len = 0;
        
    while(1)
    {
        if(_addr_start > _addr_end -3 - len)  // The sector has been searched and there is no free space
        {
            if(dirty_data_len > 0)  // Dirty data exists in the current sector
            {
                tinyFlash_Swap();  // Swap old and new sectors and clean dirty data

                _addr_start = tinyFlash_Used_Addr + TINY_SECHAD_SIZE;;
                _addr_end   = tinyFlash_Used_Addr + TINY_SECTOR_SIZE;
            }
            else  // No space available
            {
                return -1;
            }
        }

        g_addr = _addr_start;
        
        flash_read_page(_addr_start, TINY_BUFFER_SIZE, _buf);

        if(_buf[0] == 0xFF)  // This area is available
        {
            _buf[0] = KEY;
            _buf[1] = (KEY ^ 0xFF);
            _buf[2] = len;
            memcpy(_buf +3, buf, len);

            flash_write(_addr_start, len + 3, _buf);
            
            return 0;
        }
        else if(_buf[0] != 0)  // Other KEYs have been stored
        {
            _addr_start += (_buf[2] + 3);
            
            if(_buf[1] != (KEY ^ 0xFF))
            {
                dirty_data_len += (_buf[2] + 3);
            }
            continue;
        }
        else  // Wrong data read
        {
            break;
        }
    }
    
    return 0;
}

void tinyFlash_Swap()  // The sector is used up and the data needs to be cleared before other data can be stored.
{
    unsigned long _addr_start = tinyFlash_Used_Addr + TINY_SECHAD_SIZE;  // Starting address of the currently used sector
    unsigned long _addr_end   = tinyFlash_Used_Addr + TINY_SECTOR_SIZE;  // The end address of the currently used sector

    unsigned long _new_addr_start = tinyFlash_Swap_Addr + TINY_SECHAD_SIZE;  // The starting address of the sector to be used
    // unsigned long _new_addr_end = tinyFlash_Swap_Addr + TINY_SECTOR_SIZE;  // The end address of the sector to be used

    unsigned long tmp = 0;

    while(1)
    {
        if(_addr_start > _addr_end -3)  // The sector has been searched and there is no free space
        {
            break;
        }
        
        flash_read_page(_addr_start, TINY_BUFFER_SIZE, _buf);

        if(_buf[0] == 0xFF)  // Data transfer completed
        {
            break;
        }
        else if(_buf[0] == 0)  // Wrong data was read
        {
            break;
        }
        else  // Read the correct Key data
        {
            _addr_start += (_buf[2] + 3);

            if(_buf[1] == (_buf[0] ^ 0xFF))  // The data is still valid (not deleted)
            {
                tmp = _new_addr_start & 0xff;

                flash_write(_new_addr_start, _buf[2] + 3, _buf);
                _new_addr_start += (_buf[2] + 3);
            }
            continue;
        }
    }

    _buf[0] = 0xaa;
    flash_write_page(tinyFlash_Swap_Addr, 1, _buf);  // Mark new sectors as in use
    flash_erase_sector(tinyFlash_Used_Addr);  // Erase old sectors

    _new_addr_start = tinyFlash_Swap_Addr;

    tinyFlash_Swap_Addr = tinyFlash_Used_Addr;
    tinyFlash_Used_Addr = _new_addr_start;
}

/* Erase all sectors */
void tinyFlash_Format(void)
{
    flash_erase_sector(tinyFlash_Used_Addr);  // Erase old sectors
    flash_erase_sector(tinyFlash_Swap_Addr);  // Erase old sectors
} 

/* Read data from a certain area and store it in global variables, mainly used for debugging */
void tinyFlash_Debug(unsigned long addr)
{
    flash_read_page(addr, TINY_BUFFER_SIZE, _buf);
} 

/**
 * @brief This function write flash.
 * @param[in]   addr the start address of the page
 * @param[in]   len the length(in byte) of content needs to write into the page
 * @param[in]   buf the start address of the content needs to write into
 * @return none
 */
void flash_write(unsigned int addr, unsigned int len, unsigned char *buf) {
	unsigned int xlen;
	while (len) {
		xlen = addr & 0xff;
		if (xlen + len > 0x100) {
			xlen = 0x100 - xlen;
			flash_write_page(addr, xlen, buf);
			len -= xlen;
			addr += xlen;
			buf += xlen;
		} else {
			flash_write_page(addr, len, buf);
			break;
		}
	}
}

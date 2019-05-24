#include <NixApplication.h>
#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include <cstdio>


std::vector< std::vector< char > > sudoMat = 
{
	{'5', '3', '.', '.', '7', '.', '.', '.', '.'},
	{'6', '.', '.', '1', '9', '5', '.', '.', '.'},
	{'.', '9', '8', '.', '.', '.', '.', '6', '.'},
	{'8', '.', '.', '.', '6', '.', '.', '.', '3'},
	{'4', '.', '.', '8', '.', '3', '.', '.', '1'},
	{'7', '.', '.', '.', '2', '.', '.', '.', '6'},
	{'.', '6', '.', '.', '.', '.', '2', '8', '.'},
	{'.', '.', '.', '4', '1', '9', '.', '.', '5'},
	{'.', '.', '.', '.', '8', '.', '.', '7', '9'}
};

class HelloWorld : public NixApplication {
	virtual bool initialize( void* _wnd, nix::IArchieve* ) {
        printf("%s", "HelloWorld is initializing!");
		uint32_t flags = 0;
		for (auto& line : sudoMat) {
			for ( auto v : line ) {
				if (v == 9) {
					if (flags & 0x1) {
						return false;
					}
					flags |= 0x1;
				}else if (v == 1) {
					if (flags & 0x2) {
						return false;
					}
					flags |= 0x2;
				}
			}
		}
		flags = 0;
		for ( uint32_t i = 0; i<9; ++i ) {
			for (auto& line : sudoMat) {
				if (line[i] == 9) {
					if (flags & 0x1) {
						return false;
					}
					flags |= 0x1;
				}else if (line[i] == 1) {
					if (flags & 0x2) {
						return false;
					}
					flags |= 0x2;
				}
			}
		}
		//
		flags = 0;
		//
		for ( uint32_t _col = 0; _col < 3; ++_col )
		{
			for (uint32_t _row = 0; _row < 3; ++_row)
			{
				uint32_t startx = _col * 3;
				uint32_t starty = _row * 3;
				for ( uint32_t y = starty; y < starty + 3; ++y)
				{
					for (uint32_t x = startx; x < startx+3; ++x )
					{
						if (sudoMat[x][y] == 9) {
							if (flags & 0x1) {
								return false;
							}
							flags |= 0x1;
						}
						else if (sudoMat[x][y] == 1) {
							if (flags & 0x2) {
								return false;
							}
							flags |= 0x2;
						}
					}
				}
			}
		}
        return true;
    }
    
	virtual void resize(uint32_t _width, uint32_t _height) {
        printf("resized!");
    }

	virtual void release() {
        printf("destroyed");
    }

	virtual void tick() {
    }

	virtual const char * title() {
        return "hello,world!";
    }
	
	virtual uint32_t rendererType() {
		return 0;
	}
};

HelloWorld theapp;

NixApplication* GetApplication() {
    return &theapp;
}
#include "archieve.h"
#include <memory.h>
#include"../string/path.h"

namespace Nix
{
    class StdFile: public IFile
    {
        friend class StdArchieve;
    private:
        FILE* _handle = nullptr;
		size_t _size = 0;
        //
        StdFile()
        {
        }
    public:

		virtual size_t size()
		{
			return _size;
		}

        virtual bool readable()
        {
            return true;
        }

        virtual bool writable()
        {
            return true;
        }
        
        virtual bool seekable()
        {
            return true;
        }

        virtual size_t read( size_t _bytes, IFile* out_ )
        {
            char chunk[64];
            size_t bytesLeft = _bytes;
            size_t roundRead = 0;
            do
            {
                roundRead = bytesLeft > sizeof(chunk) ? sizeof(chunk) : bytesLeft;
                size_t readReal = fread( chunk, 1, sizeof(chunk), _handle);
                bytesLeft -= readReal;
                out_->write( readReal, chunk );
                if( readReal != roundRead )
                    break;
            }
            while( bytesLeft );
            return _bytes - bytesLeft;
        }

        virtual size_t read( size_t _bytes, void* out_ )
        {
            return fread( out_, 1, _bytes, _handle );
        }

        virtual size_t write( size_t _bytes, IFile* _in )
        {
            char chunk[64];
            size_t bytesLeft = _bytes;
            size_t roundWrite = 0;
            do
            {
                roundWrite = bytesLeft > sizeof(chunk) ? sizeof(chunk) : bytesLeft;
                size_t writeReal = _in->read( roundWrite, chunk );
                bytesLeft -= writeReal;
                fwrite( chunk, 1, writeReal, _handle );
                if( writeReal != roundWrite )
                    break;
            }
            while( bytesLeft );
            return _bytes - bytesLeft;
        }

        virtual size_t write( size_t _bytes, const void* _in )
        {
            return fwrite(_in, 1, _bytes, _handle);
        }

        virtual size_t tell()
        {
            return ftell(_handle);
        }

        virtual bool seek( SeekFlag _flag, long _offset )
        {
            return fseek( _handle, _offset, _flag) != 0;
        }
        //
        virtual void release()
        {
            if( _handle )
                fclose( _handle );
            delete this;
        }
    };

    class MemFile: public IFile
    {
		friend IFile* CreateMemoryBuffer( void *, size_t, MemoryFreeCB _cb );
		friend IFile* CreateMemoryBuffer(size_t _length, MemoryFreeCB _cb);
    private:
        void* _raw;
        long _size;
        long _position;
        MemoryFreeCB _destructor = nullptr;
    public:
        MemFile()
        {
            _raw = nullptr;
            _size = 0;
            _position = 0;
        }
        virtual bool readable()
        {
            return true;
        }

        virtual bool writable()
        {
            return true;
        }
        
        virtual bool seekable()
        {
            return true;
        }

        virtual size_t read( size_t _bytes, IFile* out_ )
        {
            char chunk[64];
            size_t bytesLeft = _bytes;
            size_t roundRead = 0;
            do
            {
                size_t memLeft = (_size - _position);
                roundRead = bytesLeft > sizeof(chunk) ? sizeof(chunk) : bytesLeft;
                size_t readReal = roundRead > memLeft ? memLeft : roundRead;
                memcpy( chunk, (char*)_raw + _position, readReal );
                _position += readReal;
                bytesLeft -= readReal;
                out_->write( readReal, chunk );
                if( readReal != roundRead )
                    break;
            }
            while( bytesLeft );
            return _bytes - bytesLeft;
        }

        virtual size_t read( size_t _bytes, void* out_ )
        {
            size_t memLeft = (_size - _position);
            size_t readReal = _bytes > memLeft ? memLeft : _bytes; 
            memcpy( out_, (char*)_raw + _position, readReal);
            return readReal;
        }

        virtual size_t write( size_t _bytes, IFile* _in )
        {
            size_t sizeWrite = (_size - _position);
            sizeWrite = sizeWrite > _bytes? _bytes:sizeWrite;
            auto nRead = _in->read( sizeWrite,  (char*)_raw + _position);
            _position += nRead;
            return nRead;
        }

        virtual size_t write( size_t _bytes, const void* _in )
        {
            size_t sizeWrite = (_size - _position);
            sizeWrite = sizeWrite > _bytes? _bytes:sizeWrite;
            memcpy( (char*)_raw + _position, _in, sizeWrite );
            _position+=sizeWrite;
            return sizeWrite;
        }

        virtual size_t tell()
        {
            return _position;
        }

        virtual bool seek( SeekFlag _flag, long _offset )
        {
            switch( _flag)
            {
                case SeekFlag::SeekCur:
                    _position += _offset;
                    break;
                case SeekFlag::SeekEnd:
                    _position = _size + _offset;
                    break;
                case SeekFlag::SeekSet:
                    _position = _offset;
            }

            if(_position < 0 )
            {
                _position = 0;
            }
            else if( _position > _size )
            {
                _position  = _size;
            }
            return true;
        }

		virtual size_t size()
		{
			return _size;
		}

		virtual const void* constData() const { 
			return this->_raw; 
		}
        //
        virtual void release()
        {
            if( _destructor )
                _destructor( _raw );
            delete this;
        }
    };

    class StdArchieve: public IArchieve
    {
    friend IArchieve* CreateStdArchieve( const std::string& _path );
    private:
        std::string _root;
        //
        virtual IFile* open( const std::string& _path ) override;
		virtual bool save(const std::string& _path, const void * _data, size_t _length) override;
		virtual const char * root() {
			return _root.c_str();
		}
        //
        virtual void release() override;
        //
    };

    IFile* StdArchieve::open( const std::string& _path )
    {
        std::string fullpath = _root;
        fullpath.append( _path );
        auto path = FormatFilePath( fullpath );
        auto fh = fopen( path.c_str(), "rb+" );
        if( !fh )
            return nullptr;
		fseek(fh, 0, SEEK_SET);
		size_t set = ftell(fh);
		fseek(fh, 0, SEEK_END);
		size_t size = ftell(fh) - set;
		fseek(fh, 0, SEEK_SET);
        StdFile* file = new StdFile();
		file->_size = size;
        file->_handle = fh;
        return file;
    }

	bool StdArchieve::save(const std::string& _path, const void * _data, size_t _length) {
		std::string fullpath = _root;
		fullpath.append(_path);
		auto path = FormatFilePath(fullpath);
		auto fh = fopen(path.c_str(), "wb");
		if (!fh) {
			return false;
		}
		fwrite(_data, 1, _length, fh);
		fclose(fh);
		return true;
	}

    void StdArchieve::release()
    {
        delete this;
    }

    IArchieve* CreateStdArchieve( const std::string& _path )
    {
        StdArchieve* arch = new StdArchieve();
        arch->_root = FormatFilePath(_path);
		arch->_root.push_back('/');
        return arch;
    }

	IFile* CreateMemoryBuffer(void * _ptr, size_t _length, IFile::MemoryFreeCB _freeCB )
	{
		MemFile* buffer = new MemFile();
		buffer->_raw = _ptr;
		buffer->_destructor = _freeCB;
		buffer->_size = _length;
		buffer->_position = 0;
		return buffer;
	}

	IFile* CreateMemoryBuffer( size_t _length, IFile::MemoryFreeCB _freeCB)
	{
		MemFile* buffer = new MemFile();
		void * _ptr = malloc(_length);
		buffer->_raw = _ptr;
		buffer->_destructor = _freeCB;
		buffer->_size = _length;
		buffer->_position = 0;
		return buffer;
	}

    bool TextReader::openFile( Nix::IArchieve* _arch, const std::string& _filepath)
    {
        if (m_textMemory)
            m_textMemory->release();
        auto file = _arch->open(_filepath.c_str());
        if (!file)
            return false;
        m_textMemory = Nix::CreateMemoryBuffer(file->size() + 1);
        m_textMemory->write(file->size(), file);
        m_textMemory->write(1, "\0");
        file->release();
        //
        return true;
    }
    const char * TextReader::getText()
    {
        return (const char*)m_textMemory->constData();
    }

}

// -*-c++-*-

/*!
  \file gzfstream.cpp
  \brief gzip file stream Source File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa Akiyama

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gzfstream.h"

#include <string>
#include <cstdio>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

namespace rcsc {

/////////////////////////////////////////////////////////////////////

//! the implementation of file stream buffer
struct gzfilebuf_impl {

    //! file open mode flag
    std::ios_base::openmode open_mode_;

#ifdef HAVE_LIBZ
    //! gzip file
    gzFile file_;
#endif

    //! constructor
    gzfilebuf_impl()
        : open_mode_( static_cast< std::ios_base::openmode >( 0 ) )
#ifdef HAVE_LIBZ
        , file_( NULL )
#endif
      { }
};

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

*/
gzfilebuf::gzfilebuf()
    : M_impl( new gzfilebuf_impl )
    , M_buf_size( 8192 )
    , M_buf( NULL )
    , M_remained_size( 0 )
{
    //std::cerr << "create gzfilebuf" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
gzfilebuf::~gzfilebuf()
{
    //std::cerr << "start delete gzfilebuf" << std::endl;
    if ( is_open() )
    {
        close();
    }
    //std::cerr << "start delete gzfilebuf::M_buf" << std::endl;
    if ( M_buf )
    {
        delete [] M_buf;
    }
    //std::cerr << "end delete gzfilebuf" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
gzfilebuf::is_open()
{
#ifdef HAVE_LIBZ
    if ( M_impl
         && M_impl->file_ != NULL  )
    {
        //std::cerr << "gzfilebuf is open" << std::endl;
        return true;
    }
#endif
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
gzfilebuf *
gzfilebuf::open( const char * path,
                 std::ios_base::openmode mode,
                 int level, int strategy )
{
    gzfilebuf * ret = NULL;
#ifdef HAVE_LIBZ
    if ( ! M_impl )
    {
        return ret;
    }

    if ( ! this->is_open() )
    {
        bool testi = mode & std::ios_base::in;
        bool testo = mode & std::ios_base::out;
        if ( ( testi && testo )
             || ( ! testi && ! testo ) )
        {
            std::cerr << __FILE__ << ':' << __LINE__
                      << "openmode is duplicated" << std::endl;
            return ret;
        }

        std::string mode_str = makeModeString( mode, level, strategy );
        //std::cerr << "gzfilebuf::open  mode_str = [" << mode_str
        //<< "]" << std::endl;
        if ( mode_str.empty() )
        {
            return ret;
        }

        //std::cerr << "gzfilebuf::open call gzopen" << std::endl;
        M_impl->file_ = gzopen( path, mode_str.c_str() );

        if ( M_impl->file_ == NULL )
        {
            return ret;
        }

        if ( M_buf )
        {
            destroyInternalBuffer();
        }

        //std::cerr << "gzfilebuf::open allocate buffer" << std::endl;
        M_buf = new char_type[M_buf_size];

        if ( testi )
        {
            // initial end point is same to start point,
            // because no data is read at first.
            //std::cerr << "gzfilebuf::open. in mode. setg" << std::endl;
            M_remained_size = 0;
            this->setg( M_buf, M_buf, M_buf );
            M_impl->open_mode_ = std::ios_base::in;
        }

        if ( testo )
        {
            //std::cerr << "gzfilebuf::open. out mode. setp" << std::endl;
            this->setp( M_buf, M_buf + M_buf_size );
            M_impl->open_mode_ = std::ios_base::out;
        }

        ret = this;
    }
#endif
    return ret;
}

/*-------------------------------------------------------------------*/
/*!

*/
gzfilebuf *
gzfilebuf::close() throw()
{
#ifdef HAVE_LIBZ
    if ( this->is_open() )
    {
        flushBuf();
        destroyInternalBuffer();
        //std::cerr << "close gzip file" << std::endl;
        if ( ! M_impl )
        {
            //std::cerr << "impl is null" << std::endl;
            return NULL;
        }
        //std::cerr << "impl exist" << std::endl;
        if ( M_impl->file_ == NULL )
        {
            //std::cerr << "file pointer is null" << std::endl;
            return NULL;
        }
        //std::cerr << "file pointer exist" << std::endl;
        // TODO: checking close status...
        gzclose( M_impl->file_ );
        M_impl->file_ = NULL;
        M_impl->open_mode_ = static_cast< std::ios_base::openmode >( 0 );
        //std::cerr << "finish close gzip file" << std::endl;
    }
#endif
    return NULL;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
gzfilebuf::flushBuf()
{
    bool ret = false;
#ifdef HAVE_LIBZ
    if ( is_open()
         && ( M_impl->open_mode_ & std::ios_base::out )
         && pptr() )
    {
        int size = ( pptr() - pbase() ) * sizeof( char_type );

        if ( size == 0 )
        {
            ret = true;
        }

        if ( size > 0 )
        {
            if ( gzwrite( M_impl->file_, M_buf, size ) != 0 )
            {
                // gzflush( M_impl->file_, Z_SYNC_FLUSH );
                ret = true;
            }
        }

        this->setp( M_buf, M_buf+ M_buf_size );
    }
#endif
    return ret;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::string
gzfilebuf::makeModeString( std::ios_base::openmode mode,
                           int level,
                           int strategy )
{
    std::string mode_str("");
    mode_str.reserve( 6 );

    // set I/O
    bool testi = static_cast< bool >( mode & std::ios_base::in );
    bool testo = static_cast< bool >( mode & std::ios_base::out );

    if ( ( testi && testo )
         || ( ! testi && ! testo ) )
    {
        // invalid mode
        return std::string( "" );
    }

    if ( testi )
    {
        // If input mode, it is not necessary to consider
        // about compression level and strategy.
        mode_str = "rb";
        return mode_str;
    }

    if ( testo )
    {
        mode_str = "wb";
    }

    // set compression level
    if ( level == DEFAULT_COMPRESSION )
    {
        // nothing to do
    }
    else if ( NO_COMPRESSION <= level
              && level <= BEST_COMPRESSION )
    {
        char lvstr[4];
        snprintf( lvstr, 4, "%d", level );
        mode_str += lvstr;
    }
    else
    {
        // invalid compression level
        return std::string( "" );
    }

    // set strategy
    switch ( strategy ) {
    case DEFAULT_STRATEGY:
        // nothing to do
        break;
    case FILTERED:
        mode_str += "f";
        break;
    case HUFFMAN_ONLY:
        mode_str += "h";
        break;
    case RLE:
        mode_str += "R";
        break;
    default:
        // why reach here?
        break;
    }

    return mode_str;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
gzfilebuf::destroyInternalBuffer() throw()
{
    if ( M_buf )
    {
        //std::cerr << "gzfilebuf destroy buffer" << std::endl;
        delete [] M_buf;
        M_buf = NULL;
        M_remained_size = 0;
        this->setg( NULL, NULL, NULL );
        this->setp( NULL, NULL );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
std::streambuf::int_type
gzfilebuf::overflow( std::streambuf::int_type c )
{
    //std::cerr << "gzfilebuf::overflow" << std::endl;
    flushBuf();
    if ( c != traits_type::eof() )
    {
        //std::cerr << "gzfilebuf::overflow bump" << std::endl;
        *pptr() = traits_type::to_char_type( c );
        pbump( 1 );
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::streampos
gzfilebuf::seekoff( std::streamoff off,
                    std::ios_base::seekdir way,
                    std::ios_base::openmode mode )
{
    if ( way & std::ios_base::end )
    {
        //! zlib does not support seeking from 'end'.
        return -1;
    }

    if ( ! is_open() )
    {
        return -1;
    }

    if ( ! ( M_impl->open_mode_ & mode ) )
    {
        return -1;

    }

    std::streampos ret = -1;
#ifdef HAVE_LIBZ
    if ( M_impl->open_mode_ & std::ios_base::in )
    {
        if ( way & std::ios_base::beg )
        {
            ret = gzseek( M_impl->file_, off, SEEK_SET );
            //std::cerr << "seekoff in beg off = " << off
            //          << " ret = " << ret
            //          << std::endl;
            M_remained_size = 0;
            this->setg( M_buf, M_buf, M_buf );
        }

        if ( way & std::ios_base::cur )
        {
            ret = gzseek( M_impl->file_, off, SEEK_CUR );
            ret -= this->egptr() - this->gptr();
            //off_type diff = this->egptr() - this->gptr();
            //std::cerr << "seekoff in cur off = " << off
            //          << " ret = " << ret
            //          << " pointer_diff = "<< diff
            //          << " total = " << ret - diff
            //          << std::endl;
            if ( off != 0 )
            {
                M_remained_size = 0;
                this->setg( M_buf, M_buf, M_buf );
            }
        }
    }

    if ( M_impl->open_mode_ & std::ios_base::out )
    {
        this->sync();
        if ( way & std::ios_base::beg )
        {
            std::streampos cur = gztell( M_impl->file_ );
            if ( off <= cur )
            {
                ret = gzseek( M_impl->file_, off, SEEK_SET );
                this->setp( M_buf, M_buf + M_buf_size );
            }
        }

        if ( way & std::ios_base::cur )
        {
            if ( off < 0 )
            {
                ret = gzseek( M_impl->file_, off, SEEK_CUR );
                this->setp( M_buf, M_buf + M_buf_size );
            }
        }
    }
#endif
    return ret;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::streampos
gzfilebuf::seekpos( std::streampos pos,
                    std::ios_base::openmode mode )
{
    //std::cerr << "gzfilebuf::seekpos " << pos << std::endl;
    if ( ! is_open() )
    {
        return -1;
    }

    std::streampos ret = -1;
#ifdef HAVE_LIBZ
    if ( ( M_impl->open_mode_ & std::ios_base::in )
         && ( mode & std::ios_base::in ) )
    {
        //std::cerr << "seekpos in " << pos << std::endl;
        ret = gzseek( M_impl->file_, pos, SEEK_SET );
        // and reset buffer pointer to initial position
        M_remained_size = 0;
        this->setg( M_buf, M_buf, M_buf );
    }

    if ( ( M_impl->open_mode_ & std::ios_base::out )
         && ( mode & std::ios_base::out ) )
    {
        //std::cerr << "seekpos out " << pos << std::endl;
        std::streampos cur = gztell( M_impl->file_ );
        if ( pos <= cur )
        {
            ret = gzseek( M_impl->file_, pos, SEEK_SET );
            this->setp( M_buf, M_buf + M_buf_size );
        }
    }
#endif
    return ret;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::streamsize
gzfilebuf::showmanyc()
{
    //std::cerr << "gzfilebuf::showmanyc" << std::endl;
    std::streamsize ret = 0;

    if ( is_open()
         && ( M_impl->open_mode_ & std::ios_base::in ) )
    {
        ret = std::streamsize( egptr() - gptr() );
    }

    return ret;
}

/*-------------------------------------------------------------------*/
/*!

*/
int
gzfilebuf::sync()
{
    //std::cerr << "gzfilebuf::sync" << std::endl;
    return flushBuf() ? 0 : -1;
}

#if 0
/*-------------------------------------------------------------------*/
/*
  It is not necessary to override.
*/
gzfilebuf::int_type
gzfilebuf::uflow()
{
    std::cerr << "uflow" << std::endl;
    return std::streambuf::uflow();
}
gzfilebuf::int_type
gzfilebuf::uflow()
{
    std::cerr << "gzfilebuf::uflow" << std::endl;
    int_type c = traits_type::eof();
    if ( gptr() )
    {
        if ( gptr() == egptr() )
        {
            underflow();
        }
        if ( gptr() < egptr() )
        {
            c = traits_type::to_int_type( *gptr() );
        }
    }
    return c;
}
#endif

/*-------------------------------------------------------------------*/
/*!

*/
std::streambuf::int_type
gzfilebuf::underflow()
{
#ifdef HAVE_LIBZ
    //std::cerr << "gzfilebuf::underflow" << std::endl;

    if ( ! is_open() || ! gptr() )
    {
        return traits_type::eof();
    }

    if ( M_remained_size )
    {
        M_buf[0] = M_remained_char;
    }

    int read_size = gzread( M_impl->file_,
                            ( void* )( M_buf + M_remained_size ),
                            M_buf_size * sizeof( char_type ) - M_remained_size );
    if ( read_size <= 0 )
    {
        return traits_type::eof();
    }
    //std::cerr << "read_pos = " << gzseek( M_impl->file_, 0, SEEK_CUR )
    //          << std::endl;
    int total_size = read_size + M_remained_size;
    this->setg( M_buf, M_buf, M_buf + total_size / sizeof( char_type ) );
    //std::cerr << "gzfilebuf::underflow. total_size=" << total_size
    //          << "  remained_size=" << M_remained_size
    //          << "  pointer-diff=" << (int)(egptr() - gptr()) << std::endl;
    M_remained_size = total_size % sizeof( char_type );
    if ( M_remained_size )
    {
        M_remained_char = M_buf[total_size / sizeof( char_type )];
    }

    return sgetc();
#else
    return traits_type::eof();
#endif
}

#if 0
/*-------------------------------------------------------------------*/
/*
  It is not necessary to override.
*/
std::streamsize
gzfilebuf::xsgetn( gzfilebuf::char_type * s,
                   std::streamsize n )
{
    std::cerr << "xsgetn  size=" << n
              << "  pointer-diff=" << (int)(egptr() - gptr()) << std::endl;
    return std::streambuf::xsgetn( s, n );
}
std::streamsize
gzfilebuf::xsgetn( gzfilebuf::char_type* s, std::streamsize n )
{
    std::cerr << "gzfilebuf::xsgetn  gptr= " << (int)gptr()
              << "  egptr = " << (int)egptr() << std::endl;
    std::streamsize ret = 0;
    if  ( gptr() == egptr() )
    {
        underflow();
    }
    while ( ret < n
            && gptr() < egptr()
            && ! traits_type::eq_int_type( *gptr(), traits_type::eof() ) )
    {
        *s = *gptr();
        std::cerr << "gzfilebuf::xsgetn  read [" << *s << "]" << std::endl;
        ++ret;
        ++s;
        gbump( 1 );
        if ( gptr() == egptr() )
        {
            underflow();
        }
    }
    return ret;
}
#endif

#if 0
/*-------------------------------------------------------------------*/
/*
  It is not necessary to override.
*/
std::streamsize
gzfilebuf::xsputn( gzfilebuf::char_type * s,
                   std::streamsize n )
{
    std::cerr << "gzfilebuf::xsputn" << std::endl;
    std::streamsize ret = 0;
    while ( ret < n && pptr() < epptr() )
    {
        *pptr() = *s;
        ++ret;
        ++s;
        pbump( 1 );
        if ( pptr() == epptr() )
        {
            overflow( traits_type::to_int_type( *s ) );
        }
    }
    return ret;
}
#endif

///////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

*/
gzifstream::gzifstream()
    : std::istream( static_cast< std::streambuf * >( 0 ) )
    , M_file_buf()
{
    this->init( &M_file_buf );
}

/*-------------------------------------------------------------------*/
/*!

*/
gzifstream::gzifstream( const char * path )
    : std::istream( static_cast< std::streambuf * >( 0 ) )
    , M_file_buf()
{
    this->init( &M_file_buf );
    this->open( path );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
gzifstream::open( const char * path )
{
    if ( ! M_file_buf.open( path, std::ios_base::in ) )
    {
        this->setstate( std::ios_base::failbit );
    }
    else
    {
        this->clear();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
gzifstream::close()
{
    if ( ! M_file_buf.close() )
    {
        this->setstate( std::ios_base::failbit );
    }
}

///////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

*/
gzofstream::gzofstream()
    : std::ostream( static_cast< std::streambuf * >( 0 ) )
    , M_file_buf()
{
    this->init( &M_file_buf );
}

/*-------------------------------------------------------------------*/
/*!

*/
gzofstream::gzofstream( const char * path,
                        int level,
                        int strategy )
    : std::ostream( static_cast< std::streambuf * >( 0 ) )
    , M_file_buf()
{
    this->init( &M_file_buf );
    this->open( path, level, strategy );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
gzofstream::open( const char * path,
                  int level,
                  int strategy )
{
    if ( ! M_file_buf.open( path, std::ios_base::out, level, strategy ) )
    {
        this->setstate( std::ios_base::failbit );
    }
    else
    {
        this->clear();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
gzofstream::close()
{
    if ( ! M_file_buf.close() )
    {
        this->setstate( std::ios_base::failbit );
    }
}

} // end namespace

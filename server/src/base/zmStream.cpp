#include "zm.h"
#include "zmStream.h"

#include "zmConnection.h"
#include "zmEncoder.h"

/**
* @brief 
*
* @param tag
* @param id
* @param connection
* @param provider
*/
Stream::Stream( const std::string &tag, const std::string &id, Connection *connection, FeedProvider *provider ) :
    VideoConsumer( tag, id ),
    mConnection( connection ),
    mProvider( provider ),
    mEncoder( NULL )
{
}

/**
* @brief 
*
* @param tag
* @param id
* @param connection
* @param encoder
*/
Stream::Stream( const std::string &tag, const std::string &id, Connection *connection, Encoder *encoder ) :
    VideoConsumer( tag, id ),
    mConnection( connection ),
    mProvider( encoder->provider() ),
    mEncoder( encoder )
{
}

/**
* @brief 
*/
Stream::~Stream()
{
}

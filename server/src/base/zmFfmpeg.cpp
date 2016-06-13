#include "zm.h"
#include "zmFfmpeg.h"

#include "../libgen/libgenDebug.h"
#include "../libgen/libgenThread.h"

/*
** find rational approximation to given real number
** David Eppstein / UC Irvine / 8 Aug 1993
**
** With corrections from Arno Formella, May 2008
**
** http://www.ics.uci.edu/~eppstein/numth/frap.c
**/

/**
* @brief 
*
* @param input
* @param maxden
*
* @return 
*/
AVRational double2Rational( double input, long maxden )
{
    //printf( "%.3lf\n", input );
    double startx, x;
    startx = x = input;

    long m[2][2];
    /* initialize matrix */
    m[0][0] = m[1][1] = 1;
    m[0][1] = m[1][0] = 0;

    int loop = 0;
    long ai;
    /* loop finding terms until denom gets too big */
    while ( m[1][0] * ( ai = (long)x ) + m[1][1] <= maxden )
    {
        long t;
        t = m[0][0] * ai + m[0][1];
        m[0][1] = m[0][0];
        m[0][0] = t;
        t = m[1][0] * ai + m[1][1];
        m[1][1] = m[1][0];
        m[1][0] = t;
        if ( x == (double)ai )
        {
            //printf( "div/zero\n" );
            break;     // AF: division by zero
        }
        x = 1/(x - (double) ai);
        if ( x > (double)0x7FFFFFFF )
        {
            //printf( "rep fail\n" );
            break;  // AF: representation failure
        }
        loop++;
    } 
    //printf( "L:%d\n", loop );
    if ( loop == 0 )
        return( (AVRational){ (int)input, 1 } );

    /* now remaining x is between 0 and 1/ai */
    /* approx as either 0 or 1/m where m is max that will fit in maxden */
    /* first try zero */
    //printf( "%ld/%ld, error = %e\n", m[0][0], m[1][0], startx - ((double) m[0][0] / (double) m[1][0]) );

    //double error1 = startx - ((double) m[0][0] / (double) m[1][0]);
    AVRational answer1 = { m[0][0], m[1][0] };

#if 0
    /* now try other possibility */
    ai = (maxden - m[1][1]) / m[1][0];
    m[0][0] = m[0][0] * ai + m[0][1];
    m[1][0] = m[1][0] * ai + m[1][1];
    printf("%ld/%ld, error = %e\n", m[0][0], m[1][0], startx - ((double) m[0][0] / (double) m[1][0]));

    double error2 = startx - ((double) m[0][0] / (double) m[1][0]);
    AVRational answer2 = { m[0][0], m[1][0] };

    if ( error1 < error2 )
        printf( "A1\n" );
    else
        printf( "A2\n" );

    if ( error1 > error2 )
        return( answer2 );
#endif
    return( answer1 );
}

// This section extracted from ffmpeg code. Should be moved or rewritten.
/**
* @brief 
*
* @param p
* @param end
*
* @return 
*/
static const uint8_t *_h264StartCode( const uint8_t *p, const uint8_t *end )
{
    const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for ( end -= 3; p < a && p < end; p++ )
        if ( p[0] == 0 && p[1] == 0 && p[2] == 1 )
            return( p );

    for ( end -= 3; p < end; p += 4 )
    {
        uint32_t x = *(const uint32_t*)p;
        if ( (x - 0x01010101) & (~x) & 0x80808080 )
        {
            if ( p[1] == 0 )
            {
                if ( p[0] == 0 && p[2] == 1 )
                    return( p );
                if ( p[2] == 0 && p[3] == 1 )
                    return( p+1 );
            }
            if ( p[3] == 0)
            {
                if ( p[2] == 0 && p[4] == 1 )
                    return( p+2 );
                if ( p[4] == 0 && p[5] == 1 )
                    return( p+3 );
            }
        }
    }

    for ( end += 3; p < end; p++ )
        if ( p[0] == 0 && p[1] == 0 && p[2] == 1 )
            return( p );

    return( end + 3 );
}

/**
* @brief 
*
* @param p
* @param end
*
* @return 
*/
const uint8_t *h264StartCode( const uint8_t *p, const uint8_t *end )
{
    const uint8_t *out = _h264StartCode( p, end );
    if ( p<out && out<end && !out[-1] )
        out--;
    return( out );
}

/**
* @brief 
*
* @param mutex
* @param op
*
* @return 
*/
static int ffmpegLockManager( void **mutex, enum AVLockOp op ) 
{ 
    //pthread_mutex_t** pmutex = (pthread_mutex_t**) mutex;
    Mutex **ffmpegMutex = (Mutex **)mutex;
    switch (op)
    {
        case AV_LOCK_CREATE: 
            //*pmutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)); 
            //pthread_mutex_init(*pmutex, NULL);
            *ffmpegMutex = new Mutex;
            break;
        case AV_LOCK_OBTAIN:
            //pthread_mutex_lock(*pmutex);
            (*ffmpegMutex)->lock();
            break;
        case AV_LOCK_RELEASE:
            //pthread_mutex_unlock(*pmutex);
            (*ffmpegMutex)->unlock();
            break;
        case AV_LOCK_DESTROY:
            //pthread_mutex_destroy(*pmutex);
            //free(*pmutex);
            delete *ffmpegMutex;
            break;
    }
    return 0;
}

/**
* @brief 
*/
void ffmpegInit()
{
    av_log_set_level( ( dbgLevel > DBG_INF ) ? AV_LOG_DEBUG : AV_LOG_QUIET );

    av_lockmgr_register( ffmpegLockManager );

    avcodec_register_all();
    avdevice_register_all();
    av_register_all();
    avformat_network_init();
}

/**
* @brief 
*
* @param error
*
* @return 
*/
const char *avStrError( int error )
{
    static char avErrorBuffer[256] = ""; // Should be mutex protected really
    if ( av_strerror( error, avErrorBuffer, sizeof(avErrorBuffer) ) != 0 )
        Panic( "Can't find string for AVERROR %d", error );
    return( avErrorBuffer );
}

/**
* @brief 
*
* @param dict
*/
void avDumpDict( AVDictionary *dict )
{
    Info( "Dictionary" );
    const AVDictionaryEntry *entry = NULL;
    while( (entry = av_dict_get( dict, "", entry, AV_DICT_IGNORE_SUFFIX )) )
    {
        Info( "Dict: %s = %s", entry->key, entry->value );
    }
}

/**
* @brief 
*
* @param dict
* @param name
* @param value
*/
void avDictSet( AVDictionary **dict, const char *name, const char *value )
{
    int avError = av_dict_set( dict, name, value, 0 );
    if ( avError != 0 )
    {
        Error( "Unable to set dictionary value '%s' to '%s'", name, value );
        avStrError( avError );
    }
}

/**
* @brief 
*
* @param dict
* @param name
* @param value
*/
void avDictSet( AVDictionary **dict, const char *name, int value )
{
    char valueString[64] = "";
    snprintf( valueString, sizeof(valueString), "%d", value );
    avDictSet( dict, name, valueString );
}

/**
* @brief 
*
* @param dict
* @param name
* @param value
*/
void avDictSet( AVDictionary **dict, const char *name, double value )
{
    char valueString[64] = "";
    snprintf( valueString, sizeof(valueString), "%.2f", value );
    avDictSet( dict, name, valueString );
}

///< These could possibly read the presets from the ffmpeg files themselves

/// This must be called _after_ preset function below to have any effect
/**
* @brief 
*
* @param dict
* @param profile
*/
void avSetH264Profile( AVDictionary **dict, const std::string &profile )
{
    av_dict_set( dict, "profile", profile.c_str(), 0 );
}

/**
* @brief 
*
* @param dict
* @param preset
*/
void avSetH264Preset( AVDictionary **dict, const std::string &preset )
{
    av_dict_set( dict, "preset", preset.c_str(), 0 );
}

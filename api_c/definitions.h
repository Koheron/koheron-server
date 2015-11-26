/** 
 * @file definitions.h
 * 
 * @brief Koheron API library definitions
 *
 * @author Thomas Vanderbruggen <thomas@koheron.com>
 * @date 02/05/2015
 *
 * (c) Koheron 2015
 */
 
#ifndef __DEFINITIONS_H__
#define __DEFINITIONS_H__
 
/*
 * Debugging
 */

#define VERBOSE 1

#ifndef DEBUG_KOHERON_API
# define NDEBUG
#endif

#ifdef DEBUG_KOHERON_API
# define DEBUG_MSG(msg)                                 \
    fprintf(stderr,"Error in file %s at line %u: %s", 	\
        __FILE__, __LINE__, msg);
#else
# define DEBUG_MSG(msg)
#endif

/*
 * Library export
 */
 
#ifdef BUILDING_KOHERON_LIB
  #define KOHERON_LIB_EXPORT __attribute__ ((visibility ("default") ))
#else
  #define KOHERON_LIB_EXPORT
#endif
 
#endif /* __DEFINITIONS_H__ */

/*  GNU ed - The GNU line editor.
    Copyright (C) 2006-2017 Antonio Diaz Diaz.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
    Exit status: 0 for a normal exit, 1 for environmental problems
    (file not found, invalid flags, I/O errors, etc), 2 to indicate a
    corrupt or invalid input file, 3 for an internal consistency error
    (eg, bug) which caused ed to panic.
*/
/*
 * CREDITS
 *
 *      This program is based on the editor algorithm described in
 *      Brian W. Kernighan and P. J. Plauger's book "Software Tools
 *      in Pascal", Addison-Wesley, 1981.
 *
 *      The buffering algorithm is attributed to Rodney Ruddock of
 *      the University of Guelph, Guelph, Ontario.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>
#include <libguile.h>

#include "carg_parser.h"
#include "ed.h"


static const char * const Program_name = "GNU Ed";
static const char * const program_name = "ed";
static const char * const program_year = "2017";
static const char * invocation_name = 0;

static bool restricted_ = false;	/* if set, run in restricted mode */
static bool scripted_ = false;		/* if set, suppress diagnostics,
					   byte counts and '!' prompt */
static bool traditional_ = false;	/* if set, be backwards compatible */


bool restricted( void ) { return restricted_; }
bool scripted( void ) { return scripted_; }
bool traditional( void ) { return traditional_; }


static void show_help( void )
  {
  printf( "%s - The GNU line editor.\n", Program_name );
  printf( "\nUsage: %s [options] [file]\n", invocation_name );
  printf( "\nOptions:\n"
          "  -h, --help                 display this help and exit\n"
          "  -V, --version              output version information and exit\n"
          "  -G, --traditional          run in compatibility mode\n"
          "  -l, --loose-exit-status    exit with 0 status even if a command fails\n"
          "  -p, --prompt=STRING        use STRING as an interactive prompt\n"
          "  -r, --restricted           run in restricted mode\n"
          "  -s, --quiet, --silent      suppress diagnostics, byte counts and '!' prompt\n"
          "  -v, --verbose              be verbose; equivalent to the 'H' command\n"
          "Start edit by reading in 'file' if given.\n"
          "If 'file' begins with a '!', read output of shell command.\n"
          "\nExit status: 0 for a normal exit, 1 for environmental problems (file\n"
          "not found, invalid flags, I/O errors, etc), 2 to indicate a corrupt or\n"
          "invalid input file, 3 for an internal consistency error (eg, bug) which\n"
          "caused ed to panic.\n"
          "\nReport bugs to bug-ed@gnu.org\n"
          "Ed home page: http://www.gnu.org/software/ed/ed.html\n"
          "General help using GNU software: http://www.gnu.org/gethelp\n" );
  }


#define PROGVERSION "0.0.0"

static void show_version( void )
  {
  printf( "GNU %s %s\n", program_name, PROGVERSION );
  printf( "Copyright (C) 1994 Andrew L. Moore.\n"
          "Copyright (C) %s Antonio Diaz Diaz.\n", program_year );
  printf( "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
          "This is free software: you are free to change and redistribute it.\n"
          "There is NO WARRANTY, to the extent permitted by law.\n" );
  }


SCM scm_edi_show_version(SCM args)
{
	show_version();
}

void show_strerror( const char * const filename, const int errcode )
  {
  if( !scripted_ )
    {
    if( filename && filename[0] ) fprintf( stderr, "%s: ", filename );
    fprintf( stderr, "%s\n", strerror( errcode ) );
    }
  }


static void show_error( const char * const msg, const int errcode, const bool help )
  {
  if( msg && msg[0] )
    {
    fprintf( stderr, "%s: %s", program_name, msg );
    if( errcode > 0 ) fprintf( stderr, ": %s", strerror( errcode ) );
    fputc( '\n', stderr );
    }
  if( help )
    fprintf( stderr, "Try '%s --help' for more information.\n",
             invocation_name );
  }


/* return true if file descriptor is a regular file */
bool is_regular_file( const int fd )
  {
  struct stat st;
  return ( fstat( fd, &st ) != 0 || S_ISREG( st.st_mode ) );
  }


bool may_access_filename( const char * const name )
  {
  if( restricted_ &&
      ( *name == '!' || strcmp( name, ".." ) == 0 || strchr( name, '/' ) ) )
    {
    set_error_msg( "Shell access restricted" );
    return false;
    }
  return true;
  }


static void guile_inner_main(void *closure, int argc, char **argv)
{
	scm_init_guile();

	scm_c_define_gsubr("edi-show-version", 0, 0, 0,
			(SCM (*)()) scm_edi_show_version);

	scm_c_primitive_load("hello.scm");
	SCM func_symbol = scm_c_lookup("do-hello");
	SCM func = scm_variable_ref(func_symbol);
	scm_call_0(func);
	exit(0);
}

int main( const int argc, char ** argv )
{
	scm_boot_guile(argc, argv, guile_inner_main, 0);

	// never gets here
	return 0;


  int argind;
  bool loose = false;
  const struct ap_Option options[] =
    {
    { 'G', "traditional",       ap_no  },
    { 'h', "help",              ap_no  },
    { 'l', "loose-exit-status", ap_no  },
    { 'p', "prompt",            ap_yes },
    { 'r', "restricted",        ap_no  },
    { 's', "quiet",             ap_no  },
    { 's', "silent",            ap_no  },
    { 'v', "verbose",           ap_no  },
    { 'V', "version",           ap_no  },
    {  0 ,  0,                  ap_no } };

  struct Arg_parser parser;
  invocation_name = argv[0];

  if( !ap_init( &parser, argc, argv, options, 0 ) )
    { show_error( "Memory exhausted.", 0, false ); return 1; }
  if( ap_error( &parser ) )				/* bad option */
    { show_error( ap_error( &parser ), 0, true ); return 1; }

  for( argind = 0; argind < ap_arguments( &parser ); ++argind )
    {
    const int code = ap_code( &parser, argind );
    const char * const arg = ap_argument( &parser, argind );
    if( !code ) break;					/* no more options */
    switch( code )
      {
      case 'G': traditional_ = true; break;	/* backward compatibility */
      case 'h': show_help(); return 0;
      case 'l': loose = true; break;
      case 'p': set_prompt( arg ); break;
      case 'r': restricted_ = true; break;
      case 's': scripted_ = true; break;
      case 'v': set_verbose(); break;
      case 'V': show_version(); return 0;
      default : show_error( "internal error: uncaught option.", 0, false );
                return 3;
      }
    } /* end process options */

  setlocale( LC_ALL, "" );
  if( !init_buffers() ) return 1;

  //do_guile_test();

  while( argind < ap_arguments( &parser ) )
    {
    const char * const arg = ap_argument( &parser, argind );
    if( strcmp( arg, "-" ) == 0 ) { scripted_ = true; ++argind; continue; }
    if( may_access_filename( arg ) )
      {
      if( read_file( arg, 0 ) < 0 && is_regular_file( 0 ) )
        return 2;
      else if( arg[0] != '!' ) set_def_filename( arg );
      }
    else
      {
      fputs( "?\n", stdout );
      if( arg[0] ) set_error_msg( "Invalid filename" );
      if( is_regular_file( 0 ) ) return 2;
      }
    break;
    }
  ap_free( &parser );

  return main_loop( loose );
  }

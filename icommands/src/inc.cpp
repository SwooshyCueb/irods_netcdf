/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * inc - The irods NETCDF utility
*/

#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "ncUtil.hpp"
#include "apiHandler.hpp"

void usage();

int
main( int argc, char **argv ) {
    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    char *optStr;
    rodsPathInp_t rodsPathInp;


    optStr = "hlo:R:wZ";

    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs );

    if ( status < 0 ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    if ( argc - optind <= 0 ) {
        rodsLog( LOG_ERROR, "inc: no input" );
        printf( "Use -h for help.\n" );
        exit( 2 );
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    status = parseCmdLinePath( argc, argv, optind, &myEnv,
                               UNKNOWN_OBJ_T, NO_INPUT_T, 0, &rodsPathInp );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: parseCmdLinePath error. " );
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 1, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
        exit( 7 );
    }

    status = ncUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );

    rcDisconnect( conn );

    if ( status < 0 ) {
        exit( 3 );
    }
    else {
        exit( 0 );
    }

}

void
usage() {

    char *msgs[] = {
        "Usage : inc [-hr] [--header] [--dim] [--ascitime] [--noattr] [-o outFile]",
        "[--var 'var1 var2 ...'] [--subset 'dimName1[start%stride%end] ...']|",
        "[--SUBSET dimName1[startVal%stride%endVal]...] dataObj|collection ... ",
        "Usage : inc --agginfo [-l]|[w] [-R resource] collection ...",
        " ",
        "Perform NETCDF operations on the input data objects. The data objects must",
        "be in NETCDF file format.",
        " ",
        "The -o option specifies the extracted variable values will be put into the",
        "given outFile in NETCDF format instead of text format to the terminal.",
        "The --noattr specifies that attributes will not be saved or displayed.",
        "By default, the attributes info is automatically extracted and saved to the",
        "NETCDF output file if the -o option is used or displayed to the terminal",
        "if the --header option is used.",

        "If the -o option is not used, the output will be in plain text.",
        "If no option is specified, the header info will be output.",
        " ",
        "The --var option can be used to specify a list of variables for data output",
        "to the terminal in text format or to the outFile in NETCDF format if the -o",
        "option is used. e.g., ",
        "   inc --var 'pressure temperature current' myfile.nc",
        "A value of 'all' for --var means all variables. In addition, if the ",
        "-o option is used, no --var input also means all variables.",
        " ",
        "The --subset option can be used to specify a list of subsetting conditions.",
        "Each subetting condition must be given in the form dimName[start%stride%end]",
        "where 'start' and 'end' are the starting and ending indices of the dimension",
        "array. e.g.,",
        "   inc --var pressure --subset 'longitude[2%1%8] latitude[4%1%5] time[2%1%4]'",
        "   myfile.nc",
        " ",
        "Alternatively, the --SUBSET option can be used to specify subsetting",
        "conditions using the values of the dimension valuable instead of using",
        "the dimension indeces. e.g.,",
        "   inc --SUBSET 'latitude[37.87%1%45.34] time[1146515400%1%1146533400]' myfile.nc",
        "or using ASCI value for time:",
        "   inc --SUBSET 'latitude[37.87%1%45.34] time[2006-05-01T13:30:00%1%200-05-01T18:30:00]' myfile.nc",
        " ",
        "If the specified path is a collection and the -r option is not used, the",
        "input collection is treated as an 'aggregate collection'.",
        "An 'aggregate collection' is a collection containing multiple NETCDF files",
        "in a time series. The time dimenion that ties these files together must",
        "have a case insensitive name 'time' containing the number of seconds",
        "since the Epoch in values. Before the collection can be used as an 'aggregate",
        "collection', a file named '.aggInfo' which contains info such as starting",
        "time and ending time of each NETCDF files in XML format sorted in ascending",
        "time order must exist. This file is generated automatically by the time",
        "series archival command 'incarch' or can be generated using the",
        "'inc --agginfo -w myCollection' command. Each NETCDF file must have a",
        "'netcdf' datatype (e.g., using isysmeta mod datatype' command or",
        "automatically generated by the archival operation using 'incarch') for it",
        "to be recognized as part of the aggregate file pool. If the",
        "'inc --agginfo -w' command is used to generated the .agginfo file and",
        "a file in the 'aggregate collection' is not a 'netcdf' datatype, it is",
        "excluded from the aggInfo while the generation of aggInfo continues.",
        "Then a NETCDF_INVALID_DATA_TYPE error is returned after the operation",
        "finished. Once the .aggInfo file has been generated, the 'aggregate",
        "collection' path will be treated as a single NETCDF file spanning the",
        "entire time series by many of the NETCDF APIs and this command.",
        " ",
        "Options are:",
        "-o outFile - the extracted variable values will be put into the given",
        "      outFile in NETCDF format.",
        " -r  recursive operation on the collection",
        "--agginfo - set or list the .aggInfo file of an aggregate collection",
        " -l  list (cat) the content of the .aggInfo file to std out. Can only be",
        "     used with --agginfo",
        " -w  Get the aggregate info of the aggregate collection and write it",
        "     into the .aggInfo file. Can only be used with --agginfo",
        " -R  resource - specifies the resource to store the .aggInfo file. Can",
        "     only be used with --agginfo and -w options.",
        "--ascitime - For 'time' variable, output time in asci local time instead of ",
        "      integer. e.g., 2006-05-01T08:30:00.",
        "--header - output the header info (info on atrributes, dimensions and",
        "      variables).",
        "--dim - output the values of dimension variables.",
        "--noattr - attributes will not be saved nor displayed.",
        "--var 'var1 var2 ...' - list of variables or data output. A value of 'all'",
        "      means all variables",
        "--subset 'dimName1[start%stride%end] ...' - list of subsetting conditions in",
        "      the form dimName[start%stride%end] where 'start' and 'end' are the",
        "      starting and ending indices of the dimension array",
        "--SUBSET 'dimName1[startVal%stride%endVal] ...' - list of subsetting",
        "      conditions in the form dimName[startVal%stride%endVal] where 'startVal'",
        "      and 'endVal' are the starting and ending values of the dimension array",
        " -h  this help",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "inc" );
}

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See ncOpen.h for a description of this API call.*/

#include "ncOpen.hpp"
#include "ncClose.hpp"
#include "rodsLog.hpp"
#include "dataObjOpen.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "rsApiHandler.hpp"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "getRemoteZoneResc.hpp"
#include "miscServerFunct.hpp"


int
rsNcOpenDataObj( rsComm_t *rsComm, ncOpenInp_t *ncOpenInp, int **ncid ) {
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    int status;
    dataObjInp_t dataObjInp;
    int l1descInx, myncid;

    bzero( &dataObjInp, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, ncOpenInp->objPath, MAX_NAME_LEN );
    replKeyVal( &ncOpenInp->condInput, &dataObjInp.condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, &dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return ( remoteFlag );
    }
    else if ( remoteFlag == LOCAL_HOST ) {
        addKeyVal( &dataObjInp.condInput, NO_OPEN_FLAG_KW, "" );
        if ( getValByKey( &ncOpenInp->condInput, NO_STAGING_KW ) != NULL ) {
            addKeyVal( &dataObjInp.condInput, NO_STAGING_KW, "" );
        }

        l1descInx = _rsDataObjOpen( rsComm, &dataObjInp );
        clearKeyVal( &dataObjInp.condInput );
        if ( l1descInx < 0 ) {
            return l1descInx;
        }
        remoteFlag = resoAndConnHostByDataObjInfo( rsComm,
                     L1desc[l1descInx].dataObjInfo, &rodsServerHost );
        if ( remoteFlag < 0 ) {
            return ( remoteFlag );
        }
        else if ( remoteFlag == LOCAL_HOST ) {
            status = nc_open( L1desc[l1descInx].dataObjInfo->filePath,
                              ncOpenInp->mode, &myncid );
            if ( status != NC_NOERR ) {
                rodsLog( LOG_ERROR,
                         "rsNcOpen: nc_open %s error, status = %d, %s",
                         L1desc[l1descInx].dataObjInfo->filePath, status,
                         nc_strerror( status ) );
                freeL1desc( l1descInx );
                return ( NETCDF_OPEN_ERR + status );
            }
        }
        else {
            /* execute it remotely with dataObjInfo->filePath */
            ncOpenInp_t myNcOpenInp;
            bzero( &myNcOpenInp, sizeof( myNcOpenInp ) );
            myNcOpenInp.mode = ncOpenInp->mode;
            rstrcpy( myNcOpenInp.objPath,
                     L1desc[l1descInx].dataObjInfo->filePath, MAX_NAME_LEN );
            addKeyVal( &myNcOpenInp.condInput, NATIVE_NETCDF_CALL_KW, "" );

            if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
                return status;
            }
            status = rcNcOpen( rodsServerHost->conn, &myNcOpenInp, &myncid );
            clearKeyVal( &myNcOpenInp.condInput );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "rsNcOpen: rcNcOpen %s error, status = %d",
                         myNcOpenInp.objPath, status );
                freeL1desc( l1descInx );
                return ( status );
            }
        }
        L1desc[l1descInx].l3descInx = myncid;
    }
    else {
        if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
            return status;
        }
        status = rcNcOpen( rodsServerHost->conn, ncOpenInp, &myncid );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "rsNcOpen: _rcNcOpen %s error, status = %d",
                     ncOpenInp->objPath, status );
            return ( status );
        }
        l1descInx = allocAndSetL1descForZoneOpr( myncid, &dataObjInp,
                    rodsServerHost, NULL );
    }
    if ( ncOpenInp->mode == NC_NOWRITE ) {
        L1desc[l1descInx].oprType = NC_OPEN_FOR_READ;
    }
    else {
        L1desc[l1descInx].oprType = NC_OPEN_FOR_WRITE;
    }
    *ncid = ( int * ) malloc( sizeof( int ) );
    *( *ncid ) = l1descInx;

    return 0;
}

int
rsNcOpenColl( rsComm_t *rsComm, ncOpenInp_t *ncOpenInp, int **ncid ) {
    int status;
    ncAggInfo_t *ncAggInfo = NULL;
    int l1descInx;

    status = readAggInfo( rsComm, ncOpenInp->objPath, &ncOpenInp->condInput,
                          &ncAggInfo );
    if ( status < 0 ) {
        return status;
    }

    l1descInx = allocL1desc();
    if ( l1descInx < 0 ) {
        return l1descInx;
    }
    bzero( &L1desc[l1descInx].openedAggInfo, sizeof( openedAggInfo_t ) );
    L1desc[l1descInx].openedAggInfo.ncAggInfo = ncAggInfo;
    L1desc[l1descInx].openedAggInfo.objNcid = -1;	/* not opened */
    L1desc[l1descInx].openedAggInfo.objNcid0 = -1;	/* not opened */
    status = openAggrFile( rsComm, l1descInx, 0 );
    if ( status < 0 ) {
        return status;
    }
    *ncid = ( int * ) malloc( sizeof( int ) );
    *( *ncid ) = l1descInx;

    return 0;
}

int
openAggrFile( rsComm_t *rsComm, int l1descInx, int aggElemetInx ) {
    int status, status1;
    ncOpenInp_t ncOpenInp;
    ncCloseInp_t ncCloseInp;
    openedAggInfo_t *openedAggInfo;
    int *ncid = NULL;

    openedAggInfo = &L1desc[l1descInx].openedAggInfo;
    if ( aggElemetInx > 0 && aggElemetInx == openedAggInfo->aggElemetInx ) {
        return 0;
    }
    bzero( &ncOpenInp, sizeof( ncOpenInp ) );
    rstrcpy( ncOpenInp.objPath,
             openedAggInfo->ncAggInfo->ncAggElement[aggElemetInx].objPath,
             MAX_NAME_LEN );
    status = rsNcOpenDataObj( rsComm, &ncOpenInp, &ncid );
    if ( status >= 0 ) {
        if ( aggElemetInx > 0 && openedAggInfo->aggElemetInx > 0 ) {
            bzero( &ncCloseInp, sizeof( ncCloseInp ) );
            ncCloseInp.ncid = openedAggInfo->objNcid;
            status1 = rsNcClose( rsComm, &ncCloseInp );
            if ( status1 < 0 ) {
                rodsLogError( LOG_ERROR, status1,
                              "openAndInqAggrFile: rcNcClose error for %s",
                              openedAggInfo->ncAggInfo->ncObjectName );
            }
            if ( openedAggInfo->ncInqOut != NULL ) {
                freeNcInqOut( &openedAggInfo->ncInqOut );
            }
        }
        if ( aggElemetInx == 0 ) {
            openedAggInfo->objNcid0 = *ncid;
        }
        else {
            openedAggInfo->objNcid = *ncid;
            openedAggInfo->aggElemetInx = aggElemetInx;
        }
        free( ncid );
    }
    else {
        rodsLogError( LOG_ERROR, status,
                      "openAndInqAggrFile: rsNcOpen error for %s",
                      openedAggInfo->ncAggInfo->ncAggElement[aggElemetInx].objPath );
        return status;
    }
    return status;
}

extern "C" {

    int
    rsNcOpen( rsComm_t *rsComm, ncOpenInp_t *ncOpenInp, int **ncid ) {
        int status;
        int myncid;
        specCollCache_t *specCollCache = NULL;

        if ( getValByKey( &ncOpenInp->condInput, NATIVE_NETCDF_CALL_KW ) != NULL ) {
            /* just all nc_open with objPath as file nc file path */
            /* must to be called internally */
            if ( rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH ) {
                return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
            }
            status = nc_open( ncOpenInp->objPath, ncOpenInp->mode, &myncid );
            if ( status == NC_NOERR ) {
                *ncid = ( int * ) malloc( sizeof( int ) );
                *( *ncid ) = myncid;
                return 0;
            }
            else {
                rodsLog( LOG_ERROR,
                         "rsNcOpen: nc_open %s error, status = %d, %s",
                         ncOpenInp->objPath, status, nc_strerror( status ) );
                return ( NETCDF_OPEN_ERR + status );
            }
        }
        resolveLinkedPath( rsComm, ncOpenInp->objPath, &specCollCache,
                           &ncOpenInp->condInput );

        if ( isColl( rsComm, ncOpenInp->objPath, NULL ) >= 0 ) {
            status = rsNcOpenColl( rsComm, ncOpenInp, ncid );
        }
        else {
            status = rsNcOpenDataObj( rsComm, ncOpenInp, ncid );
        }
        return status;
    }

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::api_entry* plugin_factory(
        const std::string& _inst_name,
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create a api def object
        irods::apidef_t def = { NC_OPEN_AN,
                                RODS_API_VERSION,
                                REMOTE_USER_AUTH,
                                REMOTE_USER_AUTH,
                                "NcOpenInp_PI", 0,
                                "INT_PI", 0,
                                0 // null fcn ptr, handled in delay_load
                              }; 
        // =-=-=-=-=-=-=-
        // create an api object
        irods::api_entry* api = new irods::api_entry( def );

        // =-=-=-=-=-=-=-
        // assign the fcn which will handle the api call
        api->fcn_name_ = "rsNcInq";
        
        // =-=-=-=-=-=-=-
        // assign the pack struct key and value
        strncpy( api->in_pack_key, "NcOpenInp_PI", MAX_NAME_LEN );
        api->inPackInstruct = NcOpenInp_PI;
        
        //strncpy( api->in_pack_key, "", MAX_NAME_LEN );
        //api->inPackInstruct = ;
        
        // =-=-=-=-=-=-=-
        // and... were done.
        return api;

    } // plugin_factory


}; // extern "C"




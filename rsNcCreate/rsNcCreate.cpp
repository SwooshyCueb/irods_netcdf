/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjGet.h for a description of this API call.*/

#include "ncCreate.hpp"
#include "rodsLog.hpp"
#include "dataObjCreate.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "rsApiHandler.hpp"
#include "objMetaOpr.hpp"
#include "dataObjUnlink.hpp"
#include "ncClose.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "regDataObj.hpp"
#include "getRemoteZoneResc.hpp"

extern "C" {

    int
    rsNcCreate( rsComm_t *rsComm, ncOpenInp_t *ncCreateInp, int **ncid ) {
        int remoteFlag;
        rodsServerHost_t *rodsServerHost;
        specCollCache_t *specCollCache = NULL;
        int status;
        dataObjInp_t dataObjInp;
        int l1descInx, myncid;

        if ( getValByKey( &ncCreateInp->condInput, NATIVE_NETCDF_CALL_KW ) != NULL ) {
            /* just do nc_open with objPath as file nc file path */
            /* must to be called internally */
            if ( rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH ) {
                return( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
            }
            status = nc_create( ncCreateInp->objPath, ncCreateInp->mode, &myncid );
            if ( status == NC_NOERR ) {
                *ncid = ( int * ) malloc( sizeof( int ) );
                *( *ncid ) = myncid;
                return 0;
            }
            else {
                rodsLog( LOG_ERROR,
                         "rsNccreate: nc_create %s error, status = %d, %s",
                         ncCreateInp->objPath, status, nc_strerror( status ) );
                return ( NETCDF_OPEN_ERR + status );
            }
        }
        bzero( &dataObjInp, sizeof( dataObjInp ) );
        rstrcpy( dataObjInp.objPath, ncCreateInp->objPath, MAX_NAME_LEN );
        replKeyVal( &ncCreateInp->condInput, &dataObjInp.condInput );
        resolveLinkedPath( rsComm, dataObjInp.objPath, &specCollCache,
                           &dataObjInp.condInput );
        remoteFlag = getAndConnRemoteZone( rsComm, &dataObjInp, &rodsServerHost,
                                           REMOTE_OPEN );

        if ( remoteFlag < 0 ) {
            return ( remoteFlag );
        }
        else if ( remoteFlag == LOCAL_HOST ) {
            addKeyVal( &dataObjInp.condInput, NO_OPEN_FLAG_KW, "" );
            l1descInx = _rsDataObjCreate( rsComm, &dataObjInp );
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
                status = nc_create( L1desc[l1descInx].dataObjInfo->filePath,
                                    ncCreateInp->mode, &myncid );
                if ( status != NC_NOERR ) {
                    rodsLog( LOG_ERROR,
                             "rsNcCreate: nc_open %s error, status = %d, %s",
                             ncCreateInp->objPath, status, nc_strerror( status ) );
                    freeL1desc( l1descInx );
                    return ( NETCDF_CREATE_ERR + status );
                }
            }
            else {
                /* execute it remotely with dataObjInfo->filePath */
                ncOpenInp_t myNcCreateInp;
                bzero( &myNcCreateInp, sizeof( myNcCreateInp ) );
                rstrcpy( myNcCreateInp.objPath,
                         L1desc[l1descInx].dataObjInfo->filePath, MAX_NAME_LEN );
                addKeyVal( &myNcCreateInp.condInput, NATIVE_NETCDF_CALL_KW, "" );
                status = rcNcCreate( rodsServerHost->conn, &myNcCreateInp, &myncid );
                clearKeyVal( &myNcCreateInp.condInput );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR,
                             "rsNcCreate: _rcNcCreate %s error, status = %d",
                             myNcCreateInp.objPath, status );
                    freeL1desc( l1descInx );
                    return ( status );
                }
            }
            L1desc[l1descInx].l3descInx = myncid;
            /* need to reg here since NO_OPEN_FLAG_KW does not do it */
            if ( L1desc[l1descInx].dataObjInfo->specColl == NULL ) {
                status = svrRegDataObj( rsComm, L1desc[l1descInx].dataObjInfo );
                if ( status < 0 ) {
                    ncCloseInp_t myNcCloseInp;
                    bzero( &myNcCloseInp, sizeof( myNcCloseInp ) );
                    myNcCloseInp.ncid = l1descInx;
                    rsNcClose( rsComm, &myNcCloseInp );
                    l3Unlink( rsComm, L1desc[l1descInx].dataObjInfo );
                    rodsLog( LOG_ERROR,
                             "rsNcCreate: svrRegDataObj for %s failed, status = %d",
                             L1desc[l1descInx].dataObjInfo->objPath, status );
                    freeL1desc( l1descInx );
                    return ( NETCDF_CREATE_ERR + status );
                }
            }
        }
        else {
            addKeyVal( &dataObjInp.condInput, CROSS_ZONE_CREATE_KW, "" );
            status = rcNcCreate( rodsServerHost->conn, ncCreateInp, &myncid );
            /* rm it to avoid confusion */
            rmKeyVal( &dataObjInp.condInput, CROSS_ZONE_CREATE_KW );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "rsNcCreate: _rcNcCreate %s error, status = %d",
                         ncCreateInp->objPath, status );
                return ( status );
            }
            l1descInx = allocAndSetL1descForZoneOpr( myncid, &dataObjInp,
                        rodsServerHost, NULL );
        }
        L1desc[l1descInx].oprType = NC_CREATE;
        *ncid = ( int * ) malloc( sizeof( int ) );
        *( *ncid ) = l1descInx;

        return 0;
    }

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::api_entry* plugin_factory(
        const std::string& _inst_name,
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create a api def object
        irods::apidef_t def = { NC_CREATE_AN,
                                RODS_API_VERSION,
                                REMOTE_USER_AUTH,
                                REMOTE_USER_AUTH,
                                "NcOpen_PI", 0,
                                NULL, 0,
                                0
                              }; // null fcn ptr, handled in delay_load
        // =-=-=-=-=-=-=-
        // create an api object
        irods::api_entry* api = new irods::api_entry( def );

        // =-=-=-=-=-=-=-
        // assign the fcn which will handle the api call
        api->fcn_name_ = "rsNcCreate";
        
        // =-=-=-=-=-=-=-
        // assign the pack struct key and value
        strncpy( api->in_pack_key, "NcOpen_PI", MAX_NAME_LEN );
        api->inPackInstruct = NcOpenInp_PI;
        
        // =-=-=-=-=-=-=-
        // and... were done.
        return api;

    } // plugin_factory




}; // extern "C"





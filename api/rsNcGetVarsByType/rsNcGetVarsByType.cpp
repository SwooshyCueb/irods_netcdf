/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See ncGetVarsByType.h for a description of this API call.*/

#include "ncGetVarsByType.hpp"
#include "rodsLog.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "rsApiHandler.hpp"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "getRemoteZoneResc.hpp"
#include "irods_get_l1desc.hpp"

#ifdef RODS_SERVER
int
rsNcGetVarsByTypeForObj( rsComm_t *rsComm, ncGetVarInp_t *ncGetVarInp,
                         ncGetVarOut_t **ncGetVarOut ) {
    int remoteFlag;
    rodsServerHost_t *rodsServerHost = NULL;
    int l1descInx;
    ncGetVarInp_t myNcGetVarInp;
    int status = 0;

    l1descInx = ncGetVarInp->ncid;
    l1desc_t& my_desc = irods::get_l1desc( l1descInx );
    if ( my_desc.remoteZoneHost != NULL ) {
        myNcGetVarInp = *ncGetVarInp;
        myNcGetVarInp.ncid = my_desc.remoteL1descInx;

        /* cross zone operation */
        status = rcNcGetVarsByType( my_desc.remoteZoneHost->conn,
                                    &myNcGetVarInp, ncGetVarOut );
    }
    else {
        remoteFlag = resoAndConnHostByDataObjInfo( rsComm,
                     my_desc.dataObjInfo, &rodsServerHost );
        if ( remoteFlag < 0 ) {
            return ( remoteFlag );
        }
        else if ( remoteFlag == LOCAL_HOST ) {
            status = _rsNcGetVarsByType( my_desc.l3descInx,
                                         ncGetVarInp, ncGetVarOut );
            if ( status < 0 ) {
                return status;
            }
        }
        else {
            /* execute it remotely */
            myNcGetVarInp = *ncGetVarInp;
            myNcGetVarInp.ncid = my_desc.l3descInx;
            addKeyVal( &myNcGetVarInp.condInput, NATIVE_NETCDF_CALL_KW, "" );
            status = rcNcGetVarsByType( rodsServerHost->conn, &myNcGetVarInp,
                                        ncGetVarOut );
            clearKeyVal( &myNcGetVarInp.condInput );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "rsNcGetVarsByType: rcNcGetVarsByType %d for %s error, status = %d",
                         my_desc.l3descInx,
                         my_desc.dataObjInfo->objPath, status );
                return ( status );
            }
        }
    }
    return status;
}

int
rsNcGetVarsByTypeForColl( rsComm_t *rsComm, ncGetVarInp_t *ncGetVarInp,
                          ncGetVarOut_t **ncGetVarOut ) {
    int i, j, status = 0;
    int l1descInx;
    openedAggInfo_t *openedAggInfo;
    ncInqInp_t ncInqInp;
    ncGetVarInp_t myNcGetVarInp;
    rodsLong_t timeStart0, timeEnd0, curPos;
    rodsLong_t eleStart, eleEnd;
    int timeInxInVar0;
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
    char *buf, *bufPos;
    int len, curLen;
    ncGetVarOut_t *myNcGetVarOut = NULL;
    char dataType_PI[NAME_LEN];
    int dataTypeSize;

    *ncGetVarOut = NULL;
    *dataType_PI = '\0';
    l1descInx = ncGetVarInp->ncid;
    l1desc_t& my_desc = irods::get_l1desc( l1descInx );
    openedAggInfo = &my_desc.openedAggInfo;
    if ( openedAggInfo->objNcid0 == -1 ) {
        return NETCDF_AGG_ELE_FILE_NOT_OPENED;
    }
    if ( openedAggInfo->ncInqOut0 == NULL ) {
        bzero( &ncInqInp, sizeof( ncInqInp ) );
        ncInqInp.ncid = openedAggInfo->objNcid0;
        ncInqInp.paramType = NC_ALL_TYPE;
        ncInqInp.flags = NC_ALL_FLAG;
        status = rsNcInqDataObj( rsComm, &ncInqInp, &openedAggInfo->ncInqOut0 );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "rsNcGetVarsByTypeForColl: rsNcInqDataObj for %s error",
                          openedAggInfo->ncAggInfo->ncObjectName );
            return status;
        }
    }
    timeInxInVar0 = getTimeInxInVar( openedAggInfo->ncInqOut0,
                                     ncGetVarInp->varid );

    if ( timeInxInVar0 < 0 ) {
        /* no time dim */
        timeStart0 = curPos = timeEnd0 = 0;
    }
    else if ( timeInxInVar0 >= ncGetVarInp->ndim ) {
        rodsLog( LOG_ERROR,
                 "rsNcGetVarsByTypeForColl: timeInxInVar0 %d >= ndim %d",
                 timeInxInVar0, ncGetVarInp->ndim );
        return NETCDF_DIM_MISMATCH_ERR;
    }
    else {
        timeStart0 = curPos = ncGetVarInp->start[timeInxInVar0];
        timeEnd0 = timeStart0 + ncGetVarInp->count[timeInxInVar0] - 1;
    }
    eleStart = 0;
    myNcGetVarInp = *ncGetVarInp;
    bzero( start, sizeof( char ) * NC_MAX_DIMS );
    bzero( count, sizeof( char ) * NC_MAX_DIMS );
    bzero( stride, sizeof( char ) * NC_MAX_DIMS );
    myNcGetVarInp.start = start;
    myNcGetVarInp.count = count;
    myNcGetVarInp.stride = stride;
    for ( i = 0; i < ncGetVarInp->ndim; i++ ) {
        myNcGetVarInp.start[i] = ncGetVarInp->start[i];
        myNcGetVarInp.stride[i] = ncGetVarInp->stride[i];
        myNcGetVarInp.count[i] = ncGetVarInp->count[i];
    }
    len = getSizeForGetVars( ncGetVarInp );
    if ( len <= 0 ) {
        return len;
    }
    dataTypeSize = getDataTypeSize( ncGetVarInp->dataType );
    if ( dataTypeSize < 0 ) {
        return dataTypeSize;
    }
    buf = bufPos = ( char * ) calloc( len, dataTypeSize );
    curLen = 0;
    for ( i = 0; i < openedAggInfo->ncAggInfo->numFiles; i++ ) {
        eleEnd = eleStart +
                 openedAggInfo->ncAggInfo->ncAggElement[i].arraylen - 1;
        if ( curPos >= eleStart && curPos <= eleEnd ) {
            /* in range */
            if ( i != 0 && i != openedAggInfo->aggElemetInx ) {
                status = openAggrFile( rsComm, l1descInx, i );
                if ( status < 0 ) {
                    free( buf );
                    return status;
                }
                bzero( &ncInqInp, sizeof( ncInqInp ) );
                ncInqInp.ncid = openedAggInfo->objNcid;
                ncInqInp.paramType = NC_ALL_TYPE;
                ncInqInp.flags = NC_ALL_FLAG;
                status = rsNcInqDataObj( rsComm, &ncInqInp,
                                         &openedAggInfo->ncInqOut );
                if ( status < 0 ) {
                    rodsLogError( LOG_ERROR, status,
                                  "rsNcGetVarsByTypeForColl: rsNcInqDataObj error for %s",
                                  openedAggInfo->ncAggInfo->ncObjectName );
                    free( buf );
                    return status;
                }
            }
            if ( i != 0 ) {
                char *varName0 = NULL;
                myNcGetVarInp.ncid =  openedAggInfo->objNcid;
                /* varid can be different than ele 0 */
                for ( j = 0; j < openedAggInfo->ncInqOut0->nvars; j++ ) {
                    if ( openedAggInfo->ncInqOut0->var[j].id ==
                            ncGetVarInp->varid ) {
                        varName0 = openedAggInfo->ncInqOut0->var[j].name;
                        break;
                    }
                }
                if ( varName0 == NULL ) {
                    free( buf );
                    return NETCDF_DEF_VAR_ERR;
                }
                myNcGetVarInp.varid = -1;
                for ( j = 0; j < openedAggInfo->ncInqOut0->nvars; j++ ) {
                    if ( strcmp( varName0,
                                 openedAggInfo->ncInqOut0->var[j].name ) == 0 ) {
                        myNcGetVarInp.varid =
                            openedAggInfo->ncInqOut0->var[j].id;
                        break;
                    }
                }
                if ( myNcGetVarInp.varid == -1 ) {
                    free( buf );
                    return NETCDF_DEF_VAR_ERR;
                }
            }
            else {
                myNcGetVarInp.ncid =  openedAggInfo->objNcid0;
            }
            /* adjust the start, count */
            if ( timeInxInVar0 >= 0 ) {
                myNcGetVarInp.start[timeInxInVar0] = curPos - eleStart;
                if ( timeEnd0 >= eleEnd ) {
                    myNcGetVarInp.count[timeInxInVar0] = eleEnd - curPos + 1;
                }
                else {
                    myNcGetVarInp.count[timeInxInVar0] = timeEnd0 - curPos + 1;
                }
                /* adjust curPos. need to take stride into account */
                curPos += myNcGetVarInp.count[timeInxInVar0];
                if ( myNcGetVarInp.stride[timeInxInVar0] > 0 ) {
                    int mystride = myNcGetVarInp.stride[timeInxInVar0];
                    int remaine = curPos % mystride;
                    if ( remaine > 0 ) {
                        curPos = ( curPos / mystride ) * ( mystride + 1 );
                    }
                }
            }
            status = rsNcGetVarsByTypeForObj( rsComm, &myNcGetVarInp,
                                              &myNcGetVarOut );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "rsNcGetVarsByTypeForColl: rsNcGetVarsByTypeForObj %s err",
                              openedAggInfo->ncAggInfo->ncObjectName );
                free( buf );
                return status;
            }
            if ( myNcGetVarOut->dataArray->len > 0 ) {
                curLen += myNcGetVarOut->dataArray->len;
                if ( curLen > len ) {
                    rodsLog( LOG_ERROR,
                             "rsNcGetVarsByTypeForColl: curLen %d > total len %d",
                             curLen, len );
                    free( buf );
                    return NETCDF_VARS_DATA_TOO_BIG;
                }
                memcpy( bufPos, myNcGetVarOut->dataArray->buf,
                        myNcGetVarOut->dataArray->len * dataTypeSize );
                bufPos += myNcGetVarOut->dataArray->len * dataTypeSize;
                rstrcpy( dataType_PI, myNcGetVarOut->dataType_PI, NAME_LEN );
                freeNcGetVarOut( &myNcGetVarOut );
            }
        }
        if ( curPos > timeEnd0 ) {
            break;
        }
        eleStart = eleEnd + 1;
    }
    if ( status >= 0 ) {
        if ( strlen( dataType_PI ) == 0 ) {
            return status;
        }
        *ncGetVarOut = ( ncGetVarOut_t * ) calloc( 1, sizeof( ncGetVarOut_t ) );
        ( *ncGetVarOut )->dataArray = ( dataArray_t * )
                                      calloc( 1, sizeof( dataArray_t ) );
        rstrcpy( ( *ncGetVarOut )->dataType_PI, dataType_PI, NAME_LEN );
        ( *ncGetVarOut )->dataArray->len = len;
        ( *ncGetVarOut )->dataArray->type = ncGetVarInp->dataType;
        ( *ncGetVarOut )->dataArray->buf = buf;
    }
    else {
        free( buf );
    }
    return status;
}
#endif // RODS_SERVER

/* _rsNcGetVarsByType has been moved to the client because clients need it */

extern "C" {

#ifdef RODS_SERVER
    int
    rsNcGetVarsByType( rsComm_t *rsComm, ncGetVarInp_t *ncGetVarInp,
                       ncGetVarOut_t **ncGetVarOut ) {
        int l1descInx;
        int status = 0;

        if ( getValByKey( &ncGetVarInp->condInput, NATIVE_NETCDF_CALL_KW ) !=
                NULL ) {
            /* just do nc_inq_YYYY */
            status = _rsNcGetVarsByType( ncGetVarInp->ncid, ncGetVarInp,
                                         ncGetVarOut );
            return status;
        }
        l1descInx = ncGetVarInp->ncid;
        l1desc_t& my_desc = irods::get_l1desc( l1descInx );
        if ( l1descInx < 2 || l1descInx >= NUM_L1_DESC ) {
            rodsLog( LOG_ERROR,
                     "rsNcGetVarsByType: l1descInx %d out of range",
                     l1descInx );
            return ( SYS_FILE_DESC_OUT_OF_RANGE );
        }
        if ( my_desc.inuseFlag != FD_INUSE ) {
            return BAD_INPUT_DESC_INDEX;
        }
        if ( my_desc.openedAggInfo.ncAggInfo != NULL ) {
            status = rsNcGetVarsByTypeForColl( rsComm, ncGetVarInp, ncGetVarOut );
        }
        else {
            status = rsNcGetVarsByTypeForObj( rsComm, ncGetVarInp, ncGetVarOut );
        }
        return status;
    }
#endif // RODS_SERVER

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::api_entry* plugin_factory(
        const std::string& _inst_name,
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create a api def object
        irods::apidef_t def = { NC_GET_VARS_BY_TYPE_AN,
                                RODS_API_VERSION,
                                REMOTE_USER_AUTH,
                                REMOTE_USER_AUTH,
                                "NcGetVarInp_PI", 0,
                                "NcGetVarOut_PI", 0, 0, // null fcn ptr, handled in delay_load
                                clearNcGetVarInp
                              };
        // =-=-=-=-=-=-=-
        // create an api object
        irods::api_entry* api = new irods::api_entry( def );

        // =-=-=-=-=-=-=-
        // assign the fcn which will handle the api call
#ifdef RODS_SERVER
        api->fcn_name_ = "rsNcGetVarsByType";
#endif // RODS_SERVER

        // =-=-=-=-=-=-=-
        // assign the pack struct key and value
        api->in_pack_key   = "NcGetVarInp_PI";
        api->in_pack_value = NcGetVarInp_PI;

        api->out_pack_key   = "NcGetVarOut_PI";
        api->out_pack_value = NcGetVarOut_PI;

        // =-=-=-=-=-=-=-
        // and... were done.
        return api;

    } // plugin_factory

}; // extern "C"

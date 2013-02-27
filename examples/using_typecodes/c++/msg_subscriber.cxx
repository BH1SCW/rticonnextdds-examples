/* msg_subscriber.cxx

   A subscription example

   This file is derived from code automatically generated by the rtiddsgen 
   command:

   rtiddsgen -language C++ -example <arch> msg.idl

   Example subscription of type msg automatically generated by 
   'rtiddsgen'. To test them follow these steps:

   (1) Compile this file and the example publication.

   (2) Start the subscription with the command
       objs/<arch>/msg_subscriber <domain_id> <sample_count>

   (3) Start the publication with the command
       objs/<arch>/msg_publisher <domain_id> <sample_count>

   (4) [Optional] Specify the list of discovery initial peers and 
       multicast receive addresses via an environment variable or a file 
       (in the current working directory) called NDDS_DISCOVERY_PEERS. 
       
   You can run any number of publishers and subscribers programs, and can 
   add and remove them dynamically from the domain.
              
                                   
   Example:
        
       To run the example application on domain <domain_id>:
                          
       On Unix: 
       
       objs/<arch>/msg_publisher <domain_id> 
       objs/<arch>/msg_subscriber <domain_id> 
                            
       On Windows:
       
       objs\<arch>\msg_publisher <domain_id>  
       objs\<arch>\msg_subscriber <domain_id>   
              
       
modification history
------------ -------       
*/

#include <stdio.h>
#include <stdlib.h>
#ifdef RTI_VX653
#include <vThreadsData.h>
#endif
#include "msg.h"

/* We use the typecode from built-in listeners, so we don't need to include
 * this code*/
/*
#include "msgSupport.h"
#include "ndds/ndds_cpp.h"
*/

/* We are going to use the BuiltinPublicationListener_on_data_available
 * to detect the topics that are being published on the domain
 *
 * Once we have detected a new topic, we will print out the Topic Name,
 * Participant ID, DataWriter id, and Data Type.
 */
class BuiltinPublicationListener : public DDSDataReaderListener {
public:
    virtual void on_data_available(DDSDataReader* reader);
};

void BuiltinPublicationListener::on_data_available(DDSDataReader* reader)
{
    DDSPublicationBuiltinTopicDataDataReader* builtin_reader =
        (DDSPublicationBuiltinTopicDataDataReader*) reader;
    DDS_PublicationBuiltinTopicDataSeq data_seq;
    DDS_SampleInfoSeq info_seq;
    DDS_ReturnCode_t retcode;
    DDS_ExceptionCode_t exception_code;

    DDSSubscriber            *subscriber  = NULL;
    DDSDomainParticipant     *participant = NULL;

    retcode = builtin_reader->take(data_seq, info_seq, DDS_LENGTH_UNLIMITED,
                                   DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE,
                                   DDS_ANY_INSTANCE_STATE);

    if (retcode != DDS_RETCODE_OK) {
        printf("***Error: failed to access data from the built-in reader\n");
        return;
    }

    for (int i = 0; i < data_seq.length(); ++i) {
        if (info_seq[i].valid_data) {
            printf("-----\nFound topic \"%s\"\nparticipant: %08x%08x%08x\ndatawriter: %08x%08x%08x\ntype:\n",
                   data_seq[i].topic_name,
                   data_seq[i].participant_key.value[0],
                   data_seq[i].participant_key.value[1],
                   data_seq[i].participant_key.value[2],
                   data_seq[i].key.value[0],
                   data_seq[i].key.value[1],
                   data_seq[i].key.value[2]);

            if (data_seq[i].type_code == NULL) {
                printf("No type code received, perhaps increase type_code_max_serialized_length?\n");
                continue;
            }

            /* Using the type_code propagated we print the data type
             * with print_IDL(). */
            data_seq[i].type_code->print_IDL(2, exception_code);
            if (exception_code != DDS_NO_EXCEPTION_CODE) {
                printf("Error***: print_IDL returns exception code %d",
                        exception_code);
            }
        }
    }
    builtin_reader->return_loan(data_seq, info_seq);
}


/* Delete all entities */
static int subscriber_shutdown(
    DDSDomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = participant->delete_contained_entities();
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDSTheParticipantFactory->delete_participant(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Connext provides the finalize_instance() method on
       domain participant factory for people who want to release memory used
       by the participant factory. Uncomment the following block of code for
       clean destruction of the singleton. */
/*
    retcode = DDSDomainParticipantFactory::finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
        printf("finalize_instance error %d\n", retcode);
        status = -1;
    }
*/
    return status;
}

extern "C" int subscriber_main(int domainId, int sample_count)
{
    DDSDomainParticipant *participant = NULL;
    DDSSubscriber *subscriber = NULL;
    DDSTopic *topic = NULL;
    DDS_ReturnCode_t retcode;
    const char *type_name = NULL;
    int count = 0;
    struct DDS_Duration_t receive_period = {1,0};
    int status = 0;

    /* To customize the participant QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    participant = DDSTheParticipantFactory->create_participant(
        domainId, DDS_PARTICIPANT_QOS_DEFAULT, 
        NULL, DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* If you want to change the type_code_max_serialized_length
     * programmatically (e.g., to 3070) rather than using the XML file, you
     * will need to add the following lines to your code and comment out the
     * create_participant call above. */

    /*
    DDS_DomainParticipantQos participant_qos;
    retcode = DDSTheParticipantFactory->get_default_participant_qos(participant_qos);
    if (retcode != DDS_RETCODE_OK) {
        printf("get_default_participant_qos error\n");
        return -1;
    }

    participant_qos.resource_limits.type_code_max_serialized_length = 3070;

    participant = DDSTheParticipantFactory->create_participant(
        domainId, participant_qos,
        NULL, DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        subscriber_shutdown(participant);
        return -1;
    }
    */

    /* We don't actually care about receiving the samples, just the
       topic information.  To do this, we only need the builtin
       datareader for publications.
    */

    /* First get the built-in subscriber */
    DDSSubscriber *builtin_subscriber = participant->get_builtin_subscriber();
    if (builtin_subscriber == NULL) {
        printf("***Error: failed to create builtin subscriber\n");
        return 0;
    }

    /* Then get the data reader for the built-in subscriber */
    DDSPublicationBuiltinTopicDataDataReader *builtin_publication_datareader =
        (DDSPublicationBuiltinTopicDataDataReader*)
        builtin_subscriber->lookup_datareader(DDS_PUBLICATION_TOPIC_NAME);
    if (builtin_publication_datareader == NULL) {
        printf("***Error: failed to create builtin publication data reader\n");
        return 0;
    }

    /* Finally install the listener */
    BuiltinPublicationListener *builtin_publication_listener =
        new BuiltinPublicationListener();
    builtin_publication_datareader->set_listener(builtin_publication_listener,
                                                 DDS_DATA_AVAILABLE_STATUS);

    /* Main loop */
    for (count=0; (sample_count == 0) || (count < sample_count); ++count) {
        NDDSUtility::sleep(receive_period);
    }

    /* Delete all entities */
    status = subscriber_shutdown(participant);

    return status;
}

#if defined(RTI_WINCE)
int wmain(int argc, wchar_t** argv)
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */ 
    
    if (argc >= 2) {
        domainId = _wtoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = _wtoi(argv[2]);
    }
    
    /* Uncomment this to turn on additional logging
    NDDSConfigLogger::get_instance()->
        set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API, 
                                  NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */
                                  
    return subscriber_main(domainId, sample_count);
}

#elif !(defined(RTI_VXWORKS) && !defined(__RTP__)) && !defined(RTI_PSOS)
int main(int argc, char *argv[])
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = atoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = atoi(argv[2]);
    }


    /* Uncomment this to turn on additional logging
    NDDSConfigLogger::get_instance()->
        set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API, 
                                  NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */
                                  
    return subscriber_main(domainId, sample_count);
}
#endif

#ifdef RTI_VX653
const unsigned char* __ctype = *(__ctypePtrGet());

extern "C" void usrAppInit ()
{
#ifdef  USER_APPL_INIT
    USER_APPL_INIT;         /* for backwards compatibility */
#endif
    
    /* add application specific code here */
    taskSpawn("sub", RTI_OSAPI_THREAD_PRIORITY_NORMAL, 0x8, 0x150000, (FUNCPTR)subscriber_main, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
   
}
#endif


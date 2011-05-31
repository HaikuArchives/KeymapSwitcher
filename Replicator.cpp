
#include "Replicator.h"

#include <cstring>

#include <Application.h>

#include "KeymapSwitcher.h"

status_t GetReplicantView(BMessenger target, int32 uid, BMessage *reply);
status_t GetReplicantName(BMessenger target, int32 uid, BMessage *reply);
int32 GetReplicantAt(BMessenger target, int32 index);

//
void UpdateIndicator(uint32 what) {
	// tell Indicator to change icon

	BMessage	request(B_GET_PROPERTY);
	BMessenger	to;
	BMessenger	status;

	request.AddSpecifier("Messenger");
	request.AddSpecifier("Shelf");
	  
	// In the Deskbar the Shelf is in the View "Status" in Window "Deskbar"
	request.AddSpecifier("View", "Status");
	//request.AddSpecifier("Window", "Deskbar");
	request.AddSpecifier("Window", (int32)1); // the first one "Twitcher"???
	to = BMessenger(DESKBAR_SIGNATURE, -1);
	
	BMessage	reply;
	
	if (to.SendMessage(&request, &reply) == B_OK) {
		if(reply.FindMessenger("result", &status) == B_OK) {

			// enum replicant in Status view
			int32	index = 0;
			int32	uid;
			while ((uid = GetReplicantAt(status, index++)) >= B_OK) {
				BMessage	rep_info;
				if (GetReplicantName(status, uid, &rep_info) != B_OK) {
					continue;
				}
				const char *name;
				if (rep_info.FindString("result", &name) == B_OK) {
					if(strcmp(name, REPLICANT_NAME)==0) {
						BMessage rep_view;
						if (GetReplicantView(status, uid, &rep_view)==0) {
							BMessenger indicator;
							if (rep_view.FindMessenger("result", &indicator) == B_OK) {
								indicator.SendMessage(what);
							}
						} 
					}
				}
			}
		}
	}
}


//
int32 GetReplicantAt(BMessenger target, int32 index)
{
	/*
	 So here we want to get the Unique ID of the replicant at the given index
	 in the target Shelf.
	 */
	 
	BMessage	request(B_GET_PROPERTY);		// We're getting the ID property
	BMessage	reply;
	status_t	err;
	
	request.AddSpecifier("ID");					// want the ID
	request.AddSpecifier("Replicant", index);	// of the index'th replicant
	
	if ((err = target.SendMessage(&request, &reply)) != B_OK)
		return err;
	
	int32	uid;
	if ((err = reply.FindInt32("result", &uid)) != B_OK) 
		return err;
	
	return uid;
}


//
status_t GetReplicantName(BMessenger target, int32 uid, BMessage *reply)
{
	/*
	 We send a message to the target shelf, asking it for the Name of the 
	 replicant with the given unique id.
	 */
	 
	BMessage	request(B_GET_PROPERTY);
	BMessage	uid_specifier(B_ID_SPECIFIER);	// specifying via ID
	status_t	err;
	status_t	e;
	
	request.AddSpecifier("Name");		// ask for the Name of the replicant
	
	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);
	
	if ((err = target.SendMessage(&request, reply)) != B_OK)
		return err;
	
	if (((err = reply->FindInt32("error", &e)) != B_OK) || (e != B_OK))
		return err ? err : e;
	
	return B_OK;
}

//
status_t GetReplicantView(BMessenger target, int32 uid, BMessage *reply)
{
	/*
	 We send a message to the target shelf, asking it for the Name of the 
	 replicant with the given unique id.
	 */
	 
	BMessage	request(B_GET_PROPERTY);
	BMessage	uid_specifier(B_ID_SPECIFIER);	// specifying via ID
	status_t	err;
	status_t	e;
	
	request.AddSpecifier("View");		// ask for the Name of the replicant
	
	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);
	
	if ((err = target.SendMessage(&request, reply)) != B_OK)
		return err;
	
	if (((err = reply->FindInt32("error", &e)) != B_OK) || (e != B_OK))
		return err ? err : e;
	
	return B_OK;
}


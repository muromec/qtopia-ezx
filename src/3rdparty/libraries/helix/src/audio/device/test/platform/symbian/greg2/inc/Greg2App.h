/*
* ============================================================================
*  Name     : CGreg2App from Greg2App.h
*  Part of  : greg2
*  Created  : 12/11/2002 by 
*  Description:
*     Declares main application class.
*  Version  :
*  Copyright: 
* ============================================================================
*/

#ifndef GREG2APP_H
#define GREG2APP_H

// INCLUDES
#include <aknapp.h>

// CONSTANTS
// UID of the application
const TUid KUidgreg2 = { 0x0D0CC432 };

// CLASS DECLARATION

/**
* CGreg2App application class.
* Provides factory to create concrete document object.
* 
*/
class CGreg2App : public CAknApplication
    {
    
    public: // Functions from base classes
    private:

        /**
        * From CApaApplication, creates CGreg2Document document object.
        * @return A pointer to the created document object.
        */
        CApaDocument* CreateDocumentL();
        
        /**
        * From CApaApplication, returns application's UID (KUidgreg2).
        * @return The value of KUidgreg2.
        */
        TUid AppDllUid() const;
    };

#endif

// End of File


/*
* ============================================================================
*  Name     : CGreg2Document from Greg2Document.h
*  Part of  : greg2
*  Created  : 12/11/2002 by 
*  Description:
*     Declares document for application.
*  Version  :
*  Copyright: 
* ============================================================================
*/

#ifndef GREG2DOCUMENT_H
#define GREG2DOCUMENT_H

// INCLUDES
#include <akndoc.h>
   
// CONSTANTS

// FORWARD DECLARATIONS
class  CEikAppUi;

// CLASS DECLARATION

/**
*  CGreg2Document application class.
*/
class CGreg2Document : public CAknDocument
    {
    public: // Constructors and destructor
        /**
        * Two-phased constructor.
        */
        static CGreg2Document* NewL(CEikApplication& aApp);

        /**
        * Destructor.
        */
        virtual ~CGreg2Document();

    public: // New functions

    protected:  // New functions

    protected:  // Functions from base classes

    private:

        /**
        * EPOC default constructor.
        */
        CGreg2Document(CEikApplication& aApp);
        void ConstructL();

    private:

        /**
        * From CEikDocument, create CGreg2AppUi "App UI" object.
        */
        CEikAppUi* CreateAppUiL();
    };

#endif

// End of File


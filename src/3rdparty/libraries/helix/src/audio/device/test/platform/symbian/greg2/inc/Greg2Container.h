/*
* ============================================================================
*  Name     : CGreg2Container from Greg2Container.h
*  Part of  : greg2
*  Created  : 12/11/2002 by 
*  Description:
*     Declares container control for application.
*  Version  :
*  Copyright: 
* ============================================================================
*/

#ifndef GREG2CONTAINER_H
#define GREG2CONTAINER_H

// INCLUDES
#include <coecntrl.h>
   
// FORWARD DECLARATIONS
class CEikLabel;        // for example labels

// CLASS DECLARATION

/**
*  CGreg2Container  container control class.
*  
*/
class CGreg2Container : public CCoeControl, MCoeControlObserver
    {
    public: // Constructors and destructor
        
        /**
        * EPOC default constructor.
        * @param aRect Frame rectangle for container.
        */
        void ConstructL(const TRect& aRect);

        /**
        * Destructor.
        */
        ~CGreg2Container();

    public: // New functions

    public: // Functions from base classes

    private: // Functions from base classes

       /**
        * From CoeControl,SizeChanged.
        */
        void SizeChanged();

       /**
        * From CoeControl,CountComponentControls.
        */
        TInt CountComponentControls() const;

       /**
        * From CCoeControl,ComponentControl.
        */
        CCoeControl* ComponentControl(TInt aIndex) const;

       /**
        * From CCoeControl,Draw.
        */
        void Draw(const TRect& aRect) const;

       /**
        * From ?base_class ?member_description
        */
        // event handling section
        // e.g Listbox events
        void HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType);
        
    private: //data
        
        CEikLabel* iLabel;          // example label
        CEikLabel* iToDoLabel;      // example label
        CEikLabel* iToDoLabel2; 
    };

#endif

// End of File

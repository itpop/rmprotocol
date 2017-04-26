/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     Session.cpp
--
-- PROGRAM:         RMProtocol
--
-- Functions
--                  void terminateSession()
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Trista Huang
--
-- PROGRAMMER:      Trista Huang
--
-- NOTES:
-- This program is designed to perform the session control of the app.
----------------------------------------------------------------------------------------------------------------------*/
#include "Session.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:        terminateSession
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Trista Huang
--
-- PROGRAMMER:      Trista Huang
--
-- INTERFACE:       terminateSession()
--
-- RETURNS:         void
--
-- NOTES:
-- Indicates to the system that a thread has made a request to terminate (quit). It is typically
-- used in response to a WM_DESTROY message.
----------------------------------------------------------------------------------------------------------------------*/
void terminateSession() {
    PostQuitMessage(0);
}

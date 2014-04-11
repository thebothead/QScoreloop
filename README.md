QScoreloop
==========
  
Marmalade Quick extension for Scoreloop  
  
Versions:  
Marmalade SDK 7.1.0  
Scoreloop SDK 3.0.2
  
Instructions:  
1. Install the Marmalade SDK and Quick.  
2. Download the Scoreloop SDK and integrate into Quick.  
NOTE: Instructions for integrating Scoreloop with any Marmalade project are providedin the "INSTALL.txt" file that comes with the Scoreloop SDK.  These instructions should be followed for the Marmalade Quick project (quick_prebuild.mkb).  
3. Add the Marmalade Quick extension for Scoreloop.  
NOTE: This extension was developed as a user extension and is not part of the core Marmalade Quick package.  As such certain files and namespaces have "quickuser" in them so that they can be differentiated between user extensions and Marmalade Quick core extensions.  
NOTE: If this is your first time extending Marmalade Quick you should read the documentation provided by Marmalade to get your bearings here: http://docs.madewithmarmalade.com/pages/viewpage.action?pageId=3047896  
3.a) Add the provided "quickuser_scoreloop.h" and "quickuser_scoreloop.cpp" source files to the Marmalade Quick project by first adding them both to "quickuser.mkf".  
3.b) Then add the "quickuser_scoreloop.h" to "quickuser_tolua.pkg".  
3.c) Then run the "quickuser_tolua.bat" file to re-generate the LUA user extension bindings.  
3.d) Open the "quick_prebuild.mkb" in Visual Studio and re-build the Marmalade Quick binaries for your desired target platforms.  
3.e) Copy the binaries to the appropriate "target" platform folder.  
3.f) Add the "QScoreloop.lua" to your Marmalade Quick project directory.  
You should now be able to use Scoreloop in Marmalade Quick.  
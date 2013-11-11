ChronoChat Manual
===================================================================

**Step 0**: Before open ChronoChat, make sure ndnd is running on your machine. 
If you have installed `NDNx Control Center`_, check if its tray icon shows up

.. _NDNx Control Center: http://named-data.net/download/

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/ncc.png

You also need to have your default key generated and certificate installed.
To check that, you can use a command line tool **ndnsec-ls-identity**, and you should see output similar to:

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/ndnsec-ls-identity.png

If you see nothing or something different in output, please go to `NDN Certifications`_ to set up your security environment.

.. _NDN Certifications: http://ndncert.named-data.net/

**Step 1**: When you open ChronoChat App, you should see a contact panel as shown below.

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/chronochat-1.png

You should see **Setting** button at the bottom of the panel. 
You can change your identity and setting there.

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/setting-dialog.png

The default setting is derived from the key and certificate you installed in step 0. 

The left part of the panel is your contact list. 
One must be one of your contacts if she/he wants to have a chat with you.
Right now, it is empty, so let's add some contacts. 

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/chronochat-2.png

**Step 2**: Click the **Add** button in the lower left corner. 
You should see a "Browse Contact" dialog which collects all the existing contact certificate in the testbed, for example:

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/browse-contact-1.png

You can select the contact you want to add, and then click the **Add** button.
Then you should see that the selected contact has been added into your contact list.

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/chronochat-3.png

If you know a contact's certificate name, you can add it directly via **Direct Add** button.

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/browse-contact-2.png

You should see an "Add Contact" dialog, you can directly input the contact certificate name (without version number, e.g., /ndn/edu/ucla/KEY/cs/afanasev/ksk-1383327806/ID-CERT) into the **Contact** line, and then click **Fetch** button.

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/add-contact-1.png

If the certificate can be fetched and verified, the dialog will show you the description about the certificate subject.

.. image:: https://raw.github.com/bruinfish/ChronoChat-V2/master/doc/img/add-contact-2.png


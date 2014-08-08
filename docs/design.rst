Design Document
===============

Chatroom Discovery
-------------------

Function
~~~~~~~~~

While running the ChronoChat application, a list of ongoing chatrooms
is maintained with the basic chatroom information which can be
presented to the user whenever he requests. The user can select to
join a chatroom. The application will check if the user is allowed to
join the chatroom. For chatrooms with hierarchical trust model, the
user will join immediately. For chatroom with web of trust
model, if one of the user's chatroom contact is in the chatroom, the
user will send a request for invitation. Otherwise, the application
will tell the user that he is disallowed to join in the chatroom.

Detailed Design
~~~~~~~~~~~~~~~~

When a user starts the ChronoChat application, the application will
begin to discover ongoing chatrooms. Each of the participants in a
chatroom needs to maintain a list of all the participants in the
chatroom for other users to query.

Two kinds of work need to be done during the chatroom discovery
process.

Discovery
+++++++++++++

A user sends a broadcast interest to query for all the ongoing
chatrooms' information with all the existing chatroom in the exclude
filter to enumerate all the existing chatrooms and avoid retrieving
the same data packet from in-network caches. (There can be problems
when there are so many chatrooms and the exclude filter exceeds the
practical limit of NDN packet size. It is an open issue. We assume
in this version that there are not so many chatrooms and we will try
to solve the problem in later version.) No chatroom
needs to be added when generating the list for the first time .

The name of the interest can be:

``/ndn/broadcast/chronochat/chatroom-list``

Every chatroom participant maintains an interest filter under the
prefix:

``/ndn/broadcast/chronochat/chatroom-list``

When a participant in a chatroom receives the interest,
the participant will check the exclude filter and send a data packet
with all the information of the chatroom if the chatroom name is not
in the exclude filter. There might be problems when there are too many
participants in one chatroom which causes the Data packet to exceed
the practical limit of NDN packet size. However, the participants'
information is only used for a user to decide whether to join a
chatroom. Therefore, in this case, we can only send the user
information of a subset of participants.

The name of the data packet can be:

``/ndn/broadcast/chronochat/chatroom-list/chatroom-name``

Because users can obtain the chatroom name through the
data name, the data packet that the information provider sends back
only need to contains: the name of every participant and the
chatroom's trust model.

For every participant, the data structure can be defined as:
::

   Participant := PARTICIPANT-TYPE TLV-LENGTH
                         Name

For every chatroom,
::

   Chatroom := CHATROOM-TYPE TLV-LENGTH
                        TrustModel
                        Participant+

TrustModel:
::

   TrustModel := TRUST-MODEL-TYPE TLV-LENGTH
                          nonNegativeInteger

TrustModel value:

+------------------+----------------+
| TrustModel       | Assigned value |
+==================+================+
| Hierarchical     | 0              |
+------------------+----------------+
| Web of Trust     | 1              |
+------------------+----------------+


TLV assignments:

+------------------+----------------+----------------------+
| Type             | Assigned value | Assigned value (hex) |
+==================+================+======================+
| PARTICIPANT-TYPE |  128           |  0x80                |
+------------------+----------------+----------------------+
| CHATROOM-TYPE    |  129           |  0x81                |
+------------------+----------------+----------------------+
| TRUSTMODEL-TYPE  |  130           |  0x82                |
+------------------+----------------+----------------------+


After receiving a data packet, the user puts the information of the chatroom into
a list, while adding the data name into the interest's exclude
filter and resend the interest. And he will receive more data
packets with more chatrooms' information. When the interest expired
which means that there is no other chatrooms, the list of chatrooms
is completely generated.

After a specific time(discovery time), the discovery process will
start again. Discovery time can be set as a default time(e.g. 10
minutes) or set by the user.


Refreshing
+++++++++++++

To check the existence of an old chatroom, the user sends an interest
with the name:

``/ndn/broadcast/chronochat/chatroom-list/chatroom-name``

and with MustBeFresh selector to be true which is used querying for
sync data. If a participant of the certain chatroom receives the
interest, he will send a data packet with the chatroom's
information. Otherwise, if the chatroom
no longer exists, there is no data packet sent back. Therefore, if
receiving a data packet, it means that the specific chatroom still
exists. If the interest expires, the chatroom no longer exists.

Security Consideration
~~~~~~~~~~~~~~~~~~~~~~~

In the design above, it is very hard for the user to identify if the
information he received is true. Therefore, although the data
packet can be verified by hierarchical trust model or web of trust,
it can only verify that the data is the person from whom the user
intends to receive. If the data-sending-person intends to send some
fake content in the data packet, there is no way the user can tell if
the content is correct or not. Therefore, an attacker can attack the
design by sending a great number of fake chatroom information to the
network which may cause waste in traffic and even prevent a user to
receive real chatrooms' information. However, there are not many people
having the motivation to do the attack. Therefore, some assumptions
are made below.

For every chatroom, it is assumed that every participant of a
chatroom will not intend to send fake information to other users.

Under this assumption, all the data can be verified by the public key
generated according to the corresponding trust model and the security can
be guaranteed in this level.

Chatroom Discovery Protocol
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+ **Timing**

Two duration need to be defined when maintaining the list of
chatroom:

- **Refreshing time:** For every chatroom there is a refreshing time
  which measures the frequency of checking the existence. The
  refreshing time can be defined in the data packet as FreshnessPeriod
  and after the time expired, the user will send a new interest to
  check the existence of the specific chatroom. And finally, the
  chatroom list changes according to the query result.

- **Discovery time:** Discovery time is defined by the user. When
    the timer expired, the user sends an interest to query for new
    chatrooms.

+ **Chatroom List Data Structure**

For every chatroom:

::

   enum TrustModel { TRUST_MODEL_HIERARCHICAL, TRUST_MODEL_WEBOFTRUST };

   struct Chatroom
   {
     str::string name;
     vector<Name> participants;
     TrustModel trustModel;
     vector<Name> contacts;
   };


The whole chatroom list:

::

   typedef vector<Chatroom> chatroomList;

with Raw_Messages;

package Client_SMTP is

   type Port_Type is range 0 .. 2**16 - 1;

   -- TODO: Return more specific information about the error conditions below.
   type Status_Type is
     (Success,            -- The send operation succeeded.
      Connection_Failed,  -- No connection with the server was made.
      Server_Error,       -- A connection was made, but the server reported an error.
      Timed_Out);         -- The server stopped responding in a timely manner.

   -- Sends a message to an SMTP server. This procedure could take some time.
   -- TODO: Should the server name and the sender/recipient names be their own subtypes?
   procedure Send
     (Server_Name    : in  String;
      Server_Port    : in  Port_Type;
      Sender_Name    : in  String;
      Recipient_Name : in  String;   -- TODO: Should be a list of recipients.
      Message        : in  Raw_Messages.Message_Type;
      Status         : out Status_Type);

end Client_SMTP;

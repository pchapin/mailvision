with Ada.Characters.Latin_1;
with Ada.Strings.Bounded;

private with Ada.Containers.Vectors;

-- This type package provides an abstraction of low-level messages sent via SMTP.
package Raw_Messages is

   -- Raw messages can only use ASCII character.
   subtype ASCII is Character range Ada.Characters.Latin_1.NUL .. Ada.Characters.Latin_1.DEL;

   -- RFC-5322 recommends that lines no longer than 78 be used when sending messages.
   package Raw_Lines is new Ada.Strings.Bounded.Generic_Bounded_Length(Max => 78);
   subtype Line_Type is Raw_Lines.Bounded_String;

   -- The message abstraction itself.
   type Message_Type is private;

   -- Appends a line onto the end of the given message. The line must be entirely ASCII.
   procedure Append_Line(Message : in out Message_Type; Line : in Line_Type)
     with
       Pre => (for all I in 1 .. Raw_Lines.Length(Line) => Raw_Lines.Element(Line, I) in ASCII);

   -- Returns the number of lines in the message.
   function Line_Count(Message : Message_Type) return Natural;

   -- Returns the specified line.
   function Get_Line(Message : Message_Type; Line_Number : Positive) return Line_Type
     with Pre => (Line_Number <= Line_Count(Message));

private
   use type Raw_Lines.Bounded_String;
   package Message_Vectors is
     new Ada.Containers.Vectors(Index_Type => Positive, Element_Type => Line_Type);

   type Message_Type is
      record
         Lines : Message_Vectors.Vector;
      end record;

end Raw_Messages;

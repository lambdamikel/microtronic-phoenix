/*

  Microtronic Phoenix Lite (AUTHENTIC ONLY) 

  Version 1.0 (c) Jason T. Jacques, Decle, Michael A. Wessel 
  04-08-2025 

  <jtjacques@gmail.com>
  <dweeb@decle.org.uk> 
  <miacwess@gmail.com>
 
  The Busch Microtronic 2090 is (C) Busch GmbH 
  https://www.busch-modell.de/information/Microtronic-Computer.aspx 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#define VERSION "1.0" 
#define DATE "04-12-2025"  

void setup() {

  phoenix_setup();  
  
}

void loop() {
  
  while (true) {

    phoenix_clock(); 
    phoenix_loop();

  }
  
}

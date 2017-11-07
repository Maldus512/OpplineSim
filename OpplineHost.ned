//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package oppline;

import inet.node.inet.WirelessHost;
import inet.node.inet.AdhocHost;
import inet.node.inet.StandardHost;
import inet.node.inet.NodeBase;
import inet.node.inet.INetworkNode;
import inet.linklayer.ieee80211.Ieee80211Nic;

simple OpplineApp {
    parameters:
        double tob = default(1);	//Opportunistic Beacon interval (same for both)
        //double tbo = default(1);	//Beacon Observer Interval
        double txbo = normal(3.407, 0.327); //Time interval between OB to BO 
        double txob = normal(4.302, 0.524); //Time interval between BO to OB 
        bool initial_state = default(intuniform(0,1) > 0? true: false);
        int tmi = default(1);
        bool initial_msg = default(false);
}


module OpplineHost extends NodeBase
{
    parameters:
        @display("i=device/cellphone");
//        wlan[*].type = default("Ieee80211NicAP");
//		numRadios = 0;

	submodules:
	    app: OpplineApp {
	        parameters:
                @display("p=50,100;i=block/plug;is=s");
	    }
}
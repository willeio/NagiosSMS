#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <list>
#include <assert.h>


using namespace std;

string recentHostsPath = "/var/log/nagios_hostnoti/recent_hosts"; // place to save recent hosts
string destinationPhoneNumber = "49123456789"; // mobile phone number - starting with your country code !

/**
 * @brief represents an host (if more than one host is connected to this device)
 */
class host
{
	public:
		host()
		{
                        state = "2"; // 2 = UNKNOWN
		}

                /**
                 * @brief equals checks if h and this class are equal (same host)
                 * @param h host to compare
                 * @return true if both hosts are equal
                 */
		bool equals(host* h)
		{
                        if(state == h->state && msg == h->msg) // check state and message
				return true;
			return false;
		}

		string name;
		string state;
		string msg;
};

list<host*> hostList; // list of all hosts from nagios log
list<host*> recentHostList; // list of all hosts which has already been read from nagios log

ifstream recentHostsFile;
void fillRecentHostsFromFile() // fill recent host from file
{
	recentHostsFile.open(recentHostsPath.c_str());

	if (!recentHostsFile.is_open())
	{
		assert("Cannot open the recent_hosts file !!!");
		return;
	}
	
	string line;

	while(!recentHostsFile.eof())
	{
		getline(recentHostsFile, line);
                stringstream linestr; // stringstream allows to extract parts of a string that are sperated by a space
		linestr << line;
	
		string name; linestr >> name;
		string state; linestr >> state;
		string msg; linestr >> msg;

		if (!name.size() || !state.size() || !msg.size()) // only valid stuff
			continue;

		host* h = new host();
		h->name = name;
		h->state = state;
		h->msg = msg;

                recentHostList.push_back(h); // add recent host, so that a notification is not resent
	}
}

host* getRecentHostByName(string hostname)
{
	host* h = 0;
	
	for(list<host*>::iterator i = recentHostList.begin(); i != recentHostList.end(); i++)
	{
		h = *i; // get host from list from iterator

		if(h->name == hostname)
			return h; // found, return
	}

	return 0; // not found
}

host* getHostByName(string hostname)
{
	host* h = 0;

	for(list<host*>::iterator i = hostList.begin(); i != hostList.end(); i++)
	{
		h = *i; // get host from list from iterator

		if(h->name == hostname)
		{
			return h; // found, return
		}
	}

	h = new host();	// no host found, create new
	h->name = hostname; // set hostname of new host
	hostList.push_back(h); // add host to list

	return h;
}

int main()
{
        fillRecentHostsFromFile(); // get all recent hosts

        ifstream file("/var/log/nagios3/nagios.log"); // nagios log file

	if (!file.is_open())
	{
		cerr << "Unable to open nagios logfile." << endl;
		file.close();
                return 1;
	}

	string line;
	while(!file.eof())
	{
		getline(file, line);
                replace(line.begin(), line.end(), ';', ' '); // replace all ';' to ' ' as nagios uses ';' to seperate
		stringstream str; str << line;


		int logtype = 0;
                string trash;

		// ignore text like CURRENT HOST STATE and HOST ALERT first
                // get log type
                // ADD YOUR LOGTYPES HERE IF YOU WANT MORE OR DELETE A LOGTYPE
		if (line.find("HOST ALERT: ") != string::npos)
		{
                        str >> trash;
                        str >> trash;
                        str >> trash;
			logtype = 1;
		}
		else
		if (line.find("CURRENT HOST STATE: ") != string::npos)
		{
                        str >> trash;
                        str >> trash;
                        str >> trash;
                        str >> trash;
			logtype = 2;
		}
		else
			continue;


                if (!logtype) // if this line has no logtype we want, then ignore
			continue;

		string hostname; str >> hostname;
		string state; str >> state;
                str >> trash;
                str >> trash;

                // get rest, which is known as the notifications human readable message
		string rest;
		while(!str.eof())
		{
			string temp;
			str >> temp;
			rest += temp;
		}

		string logtypestr;
		switch(logtype)
		{
			case 1:
			{
				logtypestr = "ALERT";
				break;
			}

			case 2:
			{
				logtypestr = "STATE";
				break;
			}

			default:
			case 0:
			{
				continue; // ignore message, we dont filter this message
			}

		}

                // only forward critical, down and up states
                if(state.find("CRITICAL") != string::npos ||
		   state.find("DOWN") != string::npos ||
		   state.find("UP") != string::npos)
		{
			// overwrite existing values, as we only want to track the latest message
			host* h = getHostByName(hostname);
			h->state = state;
			h->msg   = rest;
		}
	}

	file.close();
        system("rm /var/log/nagios3/nagios.log"); // delete log


	// write all hosts back to a new recent_hosts file and add host as recent host
	ofstream writeRecentHosts;
	writeRecentHosts.open(recentHostsPath.c_str());
	if(writeRecentHosts.is_open())
	{
		for(list<host*>::iterator i = hostList.begin(); i != hostList.end(); i++)
		{
			host* h = *i;
			writeRecentHosts << h->name << " ";
			writeRecentHosts << h->state << " ";
			writeRecentHosts << h->msg << " ";
			writeRecentHosts << "\n";
		}

		writeRecentHosts.flush();
		writeRecentHosts.close();
	}
	else
	{
		assert("recent_hosts file could not be opened !!! Aborting.");
		exit(1);
	}



	// send all host states
	int count = 0;
	while(hostList.size()) // as long as there are hosts in the list
	{
		host* h = hostList.back(); // get last ..
		hostList.pop_back(); // .. then delete last


                // get host from "recent_hosts", do not send statuses more than once
		host* recent = getRecentHostByName(h->name);

		if(recent) // only check if there is an recent host
		{
			if(h->equals(recent)) // ignore host because status is equal with old one
				continue;			
		}


                stringstream sms; // this is the message to be sent
		sms << "To: ";
		sms << destinationPhoneNumber; // variable
		sms << "\n\n";
		sms << "hostname=" << h->name << " ";
		sms << "state=" << h->state << " ";
		sms << "msg=" << h->msg;

		stringstream smsfilename;
		smsfilename << "/var/spool/sms/outgoing/sms";
		smsfilename << count;

		ofstream smsfile;
		smsfile.open(smsfilename.str().c_str());
		if (smsfile.is_open())
		{
			smsfile << sms.str();
			smsfile.flush();
			smsfile.close();
		}

		smsfile.flush();
		smsfile.close();

		count++;
	}

	return 0;
}

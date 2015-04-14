#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <list>
#include <ctime>
#include <dirent.h>
#include <stdlib.h>

using namespace std;


string nagiosCMD = "/var/lib/nagios3/rw/nagios.cmd"; // nagios pipe to insert informations into nagios
string directory = "/var/spool/sms/incoming/"; // incoming sms
string archiveDir = "/var/log/sms_archive/"; // archive for old sms

/**
 * @brief extractValue extracts a value from a parameter of a string by the '=' char
 * @param parameter input/output for the parameter
 * @return true if value could be extracted
 */
bool extractValue(string& parameter)
{
	size_t pos = parameter.find("=");

	if (pos == string::npos)
		return false;

	pos++; // skip the '='
	string value = parameter.substr(pos);
	parameter = value;
	return true;
}

/**
 * @brief processSMS processes the sms
 * @param smsFile filename of the sms (WITHOUT the path!)
 */
void processSMS(string smsFile)
{
	ifstream str;
        string filename = directory + smsFile; // make filename by adding path & name
	str.open(filename.c_str());

	if(!str.is_open())
	{
                cerr << "SMS file '" << smsFile << "' cannot be opened!" << endl;
		return;
	}

	string trash;
	for(int i = 0; i < 12; i++)
	{
                getline(str, trash);
	}

	string hostname; str >> hostname;
	string state; str >> state;
	string msg; str >> msg;

	str.close();

        // extract all values from the sms
	if(!extractValue(logtype) ||
	   !extractValue(hostname) ||
	   !extractValue(state) ||
	   !extractValue(msg))
	{
                cerr << "SMS '" << smsFile  << "' has invalid values and could not be processed!" << endl;

		stringstream rmCMD;
		rmCMD << "rm ";
		rmCMD << filename;

                system(rmCMD.str().c_str()); // remove defect sms

		return;
	}

	stringstream moveCMD;
	moveCMD << "mv ";
	moveCMD << filename;
	moveCMD << " ";
	moveCMD << archiveDir;
	moveCMD << "/sms_archived_";
	moveCMD << time(0);

        system(moveCMD.str().c_str()); // move sms to sms archive


        // "[TIMESTAMP] PROCESS_SERVICE_CHECK_RESULT;HOST;SERVICE;STATUS;MESSAGE\n" << nagios wants this format
        int stateNr = 2; // state 2 = UNKNOWN

	if(state == "UP")
		stateNr = 0;
	if(state == "DOWN")
		stateNr = 1;
	if(state == "CRITICAL")
		stateNr = 2;

	stringstream pipe;

	pipe << "echo \"";
	pipe << "[" << time(0) << "] ";
	pipe << "PROCESS_HOST_CHECK_RESULT;";
	pipe << hostname << ";";
	pipe << stateNr << ";";
	pipe << msg << "\""; // stateOK- Everything Looks Great\n" $now > $commandfile
	pipe << " > ";
	pipe << nagiosCMD;

        system(pipe.str().c_str()); // write to nagios pipe
}

int main()
{
        // get all sms files in sms incoming dir
	list<string> fileList;

	struct dirent* entry;
	DIR* dir;
	dir = opendir(directory.c_str());

	while((entry = readdir(dir)) != NULL)
	{
		string file;
		file = entry->d_name;

		if (file != "." && file != "..")
		{
			fileList.push_back(file);
		}
	}

	while(fileList.size())
	{
		string smsFile = fileList.back();
		fileList.pop_back();
                processSMS(smsFile); // process sms
	}

	return 0;
}

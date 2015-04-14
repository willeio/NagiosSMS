# NagiosSMS
Nagios SMS is a package of two applications. Nagios HostNoti and Nagios NotiReader. With Nagios SMS it is possible to grab nagios log events and send it via SMS to the main machine which manages all nagios events, so that machines which are not connected to a network can send their events to the main nagios server.

Only for use on Linux os


Requirements: 
  SMSTools, SIM card with the ability to send sms, hardware which supports sending sms with a SIM card
  sudo rights if there are no rights to write to sms dir or reading log files


Nagios HostNoti:
  reads the nagios log file
  use on a client which has to send all events to the main machine running nagios
  
  Info:
    change the "destinationPhoneNumber" variable before starting
    use nagios_hostnoti to send host status from a client
    nagios_hostnoti does not resend events which has already been sent
    send sms to the main nagios machine


Nagios NotiReader:
  reads sms from hostnoti
  use on the main machine running nagios which contains all events from all machines
  
  Info:
    pipes all events received from sms to nagios
  
  
How to use:
  compile easily using g++, as this package uses standard c++
  run both applications as a cron job, every 5 minutes (recommended)

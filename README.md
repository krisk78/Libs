# Libs
Libraries project

I created these libraries in order to develop few console applications on Windows platform.

I often use command line applications to batch processing files before comparing them to check the non-regression of SAP Legal Change Packs (LCP's).

So far I mainly used self-made VB scripts, it does its job well, but in certain cases its slowness can become an unacceptable constraint. In example I already had to completely change the structure of data in files, and performance of an interpreted language is really poor to do this job.

My next project will be to port a script that sort files on several criterias. The existing command sort do it very fast when you have to sort on only one criteria. But creating an index file before sorting it by the sort command and rewrite the records in the expected order is far from being so fast.

To create a console application, we need to include inline help. This is the purpose of the Usage library. It allows to define the command line syntax of the application and checks if the arguments are compliant to it. This library use two other libraries, Requirements to define arguments that depend on other arguments and Conflicts to define not compatible arguments.

All of the scripts I previously wrote performed the same sequence of tasks:
  - arguments analyze and initialization of variables
  - in certain cases doing some stuff on global files
  - batch processing each file passed through the command line
  - eventually doing some final stuff on global files
I provide the library ConsoleAppFW to implement this framework. It also use the Usage library to implement arguments checks and help.

Finally, the Utils library includes few functions to perform basic tasks that are not covered by the STL.

Version history:
1.0-alpha
  First pre-release finalized to be used by the extsort application.

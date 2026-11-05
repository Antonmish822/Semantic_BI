import threading
import os
import saxonche as saxonc

from random import randint
from time import sleep


class myThread(threading.Thread):
    def __init__(self, threadID, name, counter, node, xslt_executable, saxon_proc):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.counter = counter
        self.name = name
        self.node = node
        self.xslt_executable = xslt_executable
        self.saxon_proc = saxon_proc

    def run(self):
        print("Starting " + self.name)
        run_transform(self.name, self.counter, self.node, self.xslt_executable, self.saxon_proc)
        print("Exiting " + self.name)


def run_transform(threadName, counter, node, xslt_executable, saxon_proc):
    result_file = "threading-example/result-{}.xml".format(counter)
    '''print('Transforming with', sheet_file, 'to', result_file)'''

    if xslt_executable is not None:
        try:
            '''xslt_executable.apply_templates_returning_file(xdm_value = node, output_file = result_file)'''
            sleep(randint(2, 10))
            value1 = xslt_executable.apply_templates_returning_string(xdm_value=node)
            print(value1)
        except saxonc.PySaxonApiError as ex:
            print('Message =', ex)
    else:
        print("Exception is NULL")
    saxon_proc.detach_current_thread


saxon_proc = saxonc.PySaxonProcessor(license=False)
dir = os.getcwd()
saxon_proc.set_cwd(dir)
'''xdm_node = saxon_proc.parse_xml(xml_file_name = 'threading-example/input-samples/sample-1.xml')'''
mySourceList = ['samples/data/xmark1.xml',
                    'samples/data/xmark100k.xml',
                    'samples/data/cat.xml']

'''if xdm_node is None:
        print(saxon_proc.error_message)
        exit()'''
xslt30_processor = saxon_proc.new_xslt30_processor()
myList = []
sheet_file = "samples/data/q12_1.xsl"

xslt_executable = xslt30_processor.compile_stylesheet(stylesheet_file=sheet_file)
# Create new threads
for i in range(1, 4):
    '''sheet_file = "threading-example/sheet-samples/sheet{}.xsl".format(i)'''
    print("i=", i)
    print(mySourceList[i - i])
    xdm_node = saxon_proc.parse_xml(xml_file_name=mySourceList[i - i])
    xslt_executable_clone = xslt_executable.clone()

    thread = myThread(i, "Thread-{}".format(i), i, xdm_node, xslt_executable_clone, saxon_proc)
    myList.append(thread)
    thread.daemon = True
    thread.start()

'''thread1 = myThread(1, "Thread-1", 1, xdm_node, saxon_proc)
thread2 = myThread(2, "Thread-2", 2, xdm_node, saxon_proc)
thread3 = myThread(3, "Thread-3", 3, xdm_node, saxon_proc)'''


for thread in myList:
    thread.join()

print("Exiting Main Thread")
from tkinter import *

window = Tk()

l0 = Label(window, text="Title")
l0.grid(row=0,column=0)

l0 = Label(window, text="Author")
l0.grid(row=0,column=2)

l0 = Label(window, text="Year")
l0.grid(row=1,column=0)

l0 = Label(window, text="ISBN")
l0.grid(row=1,column=2)

title_text = StringVar()
e0 = Entry(window, textvariable=title_text)
e0.grid(row=0, column=1)

author_text = StringVar()
e1 = Entry(window, textvariable=author_text)
e1.grid(row=0, column=3)

year_text = StringVar()
e2 = Entry(window, textvariable=year_text)
e2.grid(row=1, column=1)

isbn_text = StringVar()
e3 = Entry(window, textvariable=isbn_text)
e3.grid(row=1, column=3)

lb0 = Listbox(window, height=6, width=35)
lb0.grid(row=2, column=0, rowspan=6, columnspan=2)

sb0 = Scrollbar(window)
sb0.grid(row=2, column=2, rowspan=6)

lb0.configure(yscrollcommand=sb0.set)
sb0.configure(command=lb0.yview)

b0=Button(window, text="View All", width=12)
b0.grid(row=2, column=3)

b0=Button(window, text="Search Entry", width=12)
b0.grid(row=3, column=3)

b0=Button(window, text="Add Entry", width=12)
b0.grid(row=4, column=3)

b0=Button(window, text="Update Selected", width=12)
b0.grid(row=5, column=3)

b0=Button(window, text="Delete Selected", width=12)
b0.grid(row=6, column=3)

b0=Button(window, text="Close", width=12)
b0.grid(row=7, column=3)

window.mainloop()

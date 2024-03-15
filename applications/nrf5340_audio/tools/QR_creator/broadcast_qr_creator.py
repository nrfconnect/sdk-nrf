from tkinter import *
from PIL import ImageTk
import qrcode
import base64
import sys
import string

QR_STRING_START = "BLUETOOTH:UUID:184F"
QR_BN=";BN:"
QR_AT=";AT:"
QR_AD=";AD:"
QR_BI=";BI:"
QR_BC=";BC:"
QR_SQ=";SQ:"
QR_HQ=";HQ:"
QR_VS=";VS:"
QR_AS=";AS:"
QR_PI=";PI:"
QR_NS=";NS:"
QR_BS=";BS:"
QR_NB=";NB:"
QR_SM=";SM:"
QR_PM=";PM:"

qr_string = ""


def GenerateQRCodeEnter(event):
	GenerateQRCode()


def GenerateQRCode():
	global qr_string
	global qr_image
	error = False

	qr_string = ""
	if not check_bn():
		error = True
	if not check_at():
		error = True
	if not check_ad():
		error = True
	if not check_bi():
		error = True
	if not check_bc():
		error = True
	if not check_sq():
		error = True
	if not check_hq():
		error = True
	if not check_vs():
		error = True
	if not check_as():
		error = True
	if not check_pi():
		error = True
	if not check_ns():
		error = True
	if not check_bs():
		error = True
	if not check_nb():
		error = True
	if not check_sm():
		error = True
	if not check_pm():
		error = True

	if error:
		return

	qr_string += ";;"
	fields_reset()

	name = bn_entry.get()
	if len(name):
		qr = qrcode.QRCode()
		print(qr_string)
		img = qrcode.make(qr_string)
		img.save(name + "_qr.png")

		resize_image = img.resize((300, 300))
		qr_image = ImageTk.PhotoImage(resize_image)
		qr_label.configure(image=qr_image)
	else:
		bn_entry.config(fg="red")
		bn_entry.insert(0, "Mandatory field")


def fields_reset():
	bn_entry.config(state=NORMAL, fg="black")
	at_drop.config(state=NORMAL, fg="black")
	ad_entry.config(state=NORMAL, fg="black")
	bi_entry.config(state=NORMAL, fg="black")
	bc_entry.config(state=NORMAL, fg="black")
	as_entry.config(state=NORMAL, fg="black")
	pi_entry.config(state=NORMAL, fg="black")
	ns_entry.config(state=NORMAL, fg="black")
	bs_entry.config(state=NORMAL, fg="black")
	nb_entry.config(state=NORMAL, fg="black")


def close(event):
	sys.exit()


def check_bn():
	global qr_string
	name = bn_entry.get()
	if len(name):
		qr_string += QR_STRING_START + QR_BN + base64.standard_b64encode(bytes(name, 'utf-8')).decode("ascii")
		return 1
	else:
		bn_entry.config(fg="red")
		bn_entry.insert(0, "Mandatory field")
		return 0


def check_at():
	global qr_string
	address = ad_entry.get()
	if len(address):
		address_type = at_value.get()
		if address_type == "Not Set":
			print("Address type not set!!")
			at_drop.config(fg="red")
			return 0
		elif address_type == "Public":
			qr_string += QR_AT + "0"
		elif address_type == "Random":
			qr_string += QR_AT + "1"
	return 1


def check_ad():
	global qr_string
	address = ad_entry.get()

	if len(address) == 0:
		return 1

	if not all(c in string.hexdigits for c in address):
		ad_entry.config(fg="red")
		ad_entry.delete(0, END);
		ad_entry.insert(0, "Invalid hex value")
		return 0

	if len(address) != 12:
		ad_entry.config(fg="red")
		ad_entry.delete(0, END);
		ad_entry.insert(0, "Invalid length: " + str(len(address)))
		return 0

	qr_string += QR_AD + address.upper()
	return 1


def check_bi():
	global qr_string
	broadcast_id = bi_entry.get()

	if len(broadcast_id) == 0:
		return 1

	if not all(c in string.hexdigits for c in broadcast_id):
		bi_entry.config(fg="red")
		bi_entry.delete(0, END);
		bi_entry.insert(0, "Invalid hex value")
		return 0

	if len(broadcast_id) != 6:
		bi_entry.config(fg="red")
		bi_entry.delete(0, END);
		bi_entry.insert(0, "Invalid length: " + str(len(broadcast_id)))
		return 0

	broadcast_id = broadcast_id.lstrip("0")
	qr_string += QR_BI + broadcast_id.upper()
	return 1


def check_bc():
	global qr_string
	broadcast_code = bc_entry.get()

	if len(broadcast_code) == 0:
		return 1

	if len(broadcast_code)!= 16:
		bc_entry.config(fg="red")
		bc_entry.delete(0, END);
		bc_entry.insert(0, "Invalid length: " + str(len(broadcast_code)))
		return 0

	qr_string += QR_BC + base64.standard_b64encode(bytes(broadcast_code, 'utf-8')).decode("ascii")
	return 1


def check_sq():
	global qr_string
	present = 0
	sq_present = sq_value.get()

	if sq_present == "Not Set":
		return 1

	if sq_present == "Present":
		present = 1

	qr_string += QR_SQ + str(present)
	return 1


def check_hq():
	global qr_string
	present = 0
	hq_present = hq_value.get()

	if hq_present == "Not Set":
		return 1

	if hq_present == "Present":
		present = 1

	qr_string += QR_HQ + str(present)
	return 1


def check_vs():
	global qr_string
	vendor_specific = vs_entry.get()

	if len(vendor_specific) == 0:
		return 1

	qr_string += QR_VS + base64.standard_b64encode(bytes(vendor_specific, 'utf-8')).decode("ascii")
	return 1


def check_as():
	global qr_string
	advertising_id = as_entry.get()

	if len(advertising_id) == 0:
		return 1

	value = "0x" + advertising_id
	value_int = int(value, 16)

	if not all(c in string.hexdigits for c in advertising_id) or value_int > 15:
		as_entry.config(fg="red")
		as_entry.delete(0, END);
		as_entry.insert(0, "Invalid hex value")
		return 0

	if len(advertising_id) != 2:
		as_entry.config(fg="red")
		as_entry.delete(0, END);
		as_entry.insert(0, "Invalid length: " + str(len(advertising_id)))
		return 0

	advertising_id = advertising_id.lstrip("0")
	qr_string += QR_AS + advertising_id.upper()
	return 1


def check_pi():
	global qr_string
	pa_interval = pi_entry.get()

	if len(pa_interval) == 0:
		return 1

	if not all(c in string.hexdigits for c in pa_interval):
		pi_entry.config(fg="red")
		pi_entry.delete(0, END);
		pi_entry.insert(0, "Invalid hex value")
		return 0

	if len(pa_interval) != 4:
		pi_entry.config(fg="red")
		pi_entry.delete(0, END);
		pi_entry.insert(0, "Invalid length: " + str(len(pa_interval)))
		return 0

	pa_interval = pa_interval.lstrip("0")
	qr_string += QR_PI + pa_interval.upper()
	return 1


def check_ns():
	global qr_string
	num_subgroups = ns_entry.get()

	if len(num_subgroups) == 0:
		return 1

	value = "0x" + num_subgroups
	value_int = int(value, 16)

	if not all(c in string.hexdigits for c in num_subgroups) or value_int > 31 or value_int == 0:
		ns_entry.config(fg="red")
		ns_entry.delete(0, END);
		ns_entry.insert(0, "Invalid hex value")
		return 0

	if len(num_subgroups) != 2:
		ns_entry.config(fg="red")
		ns_entry.delete(0, END);
		ns_entry.insert(0, "Invalid length: " + str(len(num_subgroups)))
		return 0

	num_subgroups = num_subgroups.lstrip("0")
	qr_string += QR_NS + num_subgroups.upper()
	return 1


def check_bs():
	global qr_string
	BIS_sync = bs_entry.get()

	if len(BIS_sync) == 0:
		return 1


	if not all(c in string.hexdigits for c in BIS_sync):
		bs_entry.config(fg="red")
		bs_entry.delete(0, END);
		bs_entry.insert(0, "Invalid hex value")
		return 0

	if len(BIS_sync) > 8:
		bs_entry.config(fg="red")
		bs_entry.delete(0, END);
		bs_entry.insert(0, "Invalid length: " + str(len(BIS_sync)))
		return 0

	BIS_sync = BIS_sync.lstrip("0")
	qr_string += QR_BS + BIS_sync.upper()
	return 1


def check_nb():
	global qr_string
	SG_num_BISes = nb_entry.get()

	if len(SG_num_BISes) == 0:
		return 1

	value = "0x" + SG_num_BISes
	value_int = int(value, 16)

	if not all(c in string.hexdigits for c in SG_num_BISes) or value_int > 31 or value_int == 0:
		nb_entry.config(fg="red")
		nb_entry.delete(0, END);
		nb_entry.insert(0, "Invalid hex value")
		return 0

	if len(SG_num_BISes) != 2:
		nb_entry.config(fg="red")
		nb_entry.delete(0, END);
		nb_entry.insert(0, "Invalid length: " + str(len(SG_num_BISes)))
		return 0

	SG_num_BISes = SG_num_BISes.lstrip("0")
	qr_string += QR_NB + SG_num_BISes.upper()
	return 1


def check_sm():
	global qr_string
	SG_metadata = sm_entry.get()

	if len(SG_metadata) == 0:
		return 1

	qr_string += QR_SG + base64.standard_b64encode(bytes(SG_metadata, 'utf-8')).decode("ascii")
	return 1


def check_pm():
	global qr_string
	PM_metadata = pm_entry.get()

	if len(PM_metadata) == 0:
		return 1

	qr_string += QR_PM + base64.standard_b64encode(bytes(PM_metadata, 'utf-8')).decode("ascii")
	return 1


root = Tk()
root.title("Broadcast QR Code Creator")
root.minsize(600, 500)
root.config(bg="lightgrey")

root.bind('<Return>', GenerateQRCodeEnter)
root.bind('<Escape>', close)

qr_image = PhotoImage()
qr_label = Label(root, image=qr_image, width=300, height=300)
qr_label.grid(row=0, column=0, rowspan=6, padx=5, pady=5)

qr_btn = Button(root, text="Generate QR Code", padx=5, pady=5, command=GenerateQRCode)
qr_btn.grid(row=7, column=0, padx=5, pady=5)

Label(root, text="Broadcast Name", bg="lightgrey").grid(row=1, column=1, padx=5, pady=5, sticky=W)
bn_entry = Entry(root, bd=1)
bn_entry.grid(row=1, column=2, padx=5, pady=5)
Label(root, text="(String)", bg="lightgrey").grid(row=1, column=3, padx=5, pady=5, sticky=W)

at_value = StringVar()
at_value.set("Not Set")
at_options = ["Not Set", "Public", "Random"]
Label(root, text="Advertiser Address Type", bg="lightgrey").grid(row=2, column=1, padx=5, pady=5, sticky=W)
at_drop = OptionMenu(root, at_value, *at_options)
at_drop.grid(row=2, column=2, padx=5, pady=5)

Label(root, text="Advertiser Address", bg="lightgrey").grid(row=3, column=1, padx=5, pady=5, sticky=W)
ad_entry = Entry(root, bd=1)
ad_entry.grid(row=3, column=2, padx=5, pady=5)
Label(root, text="(12 bytes hex)", bg="lightgrey").grid(row=3, column=3, padx=5, pady=5, sticky=W)

Label(root, text="Broadcast ID", bg="lightgrey").grid(row=4, column=1, padx=5, pady=5, sticky=W)
bi_entry = Entry(root, bd=1)
bi_entry.grid(row=4, column=2, padx=5, pady=5)
Label(root, text="(6 bytes hex)", bg="lightgrey").grid(row=4, column=3, padx=5, pady=5, sticky=W)

Label(root, text="Broadcast Code", bg="lightgrey").grid(row=5, column=1, padx=5, pady=5, sticky=W)
bc_entry = Entry(root, bd=1)
bc_entry.grid(row=5, column=2, padx=5, pady=5)
Label(root, text="(String 16 char max)", bg="lightgrey").grid(row=5, column=3, padx=5, pady=5, sticky=W)

sq_value = StringVar()
sq_value.set("Not Set")
sq_options = ["Not Set", "Present", "Not Present"]
Label(root, text="Standard Quality", bg="lightgrey").grid(row=6, column=1, padx=5, pady=5, sticky=W)
sq_drop = OptionMenu(root, sq_value, *sq_options)
sq_drop.grid(row=6, column=2, padx=5, pady=5)

hq_value = StringVar()
hq_value.set("Not Set")
hq_options = ["Not Set", "Present", "Not Present"]
Label(root, text="High Quality", bg="lightgrey").grid(row=7, column=1, padx=5, pady=5, sticky=W)
hq_drop = OptionMenu(root, hq_value, *hq_options)
hq_drop.grid(row=7, column=2, padx=5, pady=5)

Label(root, text="Vendor Specific", bg="lightgrey").grid(row=8, column=1, padx=5, pady=5, sticky=W)
vs_entry = Entry(root, bd=1)
vs_entry.grid(row=8, column=2, padx=5, pady=5)
Label(root, text="(String)", bg="lightgrey").grid(row=8, column=3, padx=5, pady=5, sticky=W)

Label(root, text="Advertising ID", bg="lightgrey").grid(row=9, column=1, padx=5, pady=5, sticky=W)
as_entry = Entry(root, bd=1)
as_entry.grid(row=9, column=2, padx=5, pady=5)
Label(root, text="(00-0F)", bg="lightgrey").grid(row=9, column=3, padx=5, pady=5, sticky=W)

Label(root, text="PA Interval", bg="lightgrey").grid(row=10, column=1, padx=5, pady=5, sticky=W)
pi_entry = Entry(root, bd=1)
pi_entry.grid(row=10, column=2, padx=5, pady=5)
Label(root, text="(0000-FFFF)", bg="lightgrey").grid(row=10, column=3, padx=5, pady=5, sticky=W)

Label(root, text="Num Subgroups", bg="lightgrey").grid(row=11, column=1, padx=5, pady=5, sticky=W)
ns_entry = Entry(root, bd=1)
ns_entry.grid(row=11, column=2, padx=5, pady=5)
Label(root, text="(1-31, 2 bytes hex)", bg="lightgrey").grid(row=11, column=3, padx=5, pady=5, sticky=W)

Label(root, text="BIS Sync", bg="lightgrey").grid(row=12, column=1, padx=5, pady=5, sticky=W)
bs_entry = Entry(root, bd=1)
bs_entry.grid(row=12, column=2, padx=5, pady=5)
Label(root, text="(00000000-FFFFFFFF)", bg="lightgrey").grid(row=12, column=3, padx=5, pady=5, sticky=W)

Label(root, text="SG Number of BISes", bg="lightgrey").grid(row=13, column=1, padx=5, pady=5, sticky=W)
nb_entry = Entry(root, bd=1)
nb_entry.grid(row=13, column=2, padx=5, pady=5)
Label(root, text="(1-31, 2 bytes hex)", bg="lightgrey").grid(row=13, column=3, padx=5, pady=5, sticky=W)

Label(root, text="SG Metadata", bg="lightgrey").grid(row=14, column=1, padx=5, pady=5, sticky=W)
sm_entry = Entry(root, bd=1)
sm_entry.grid(row=14, column=2, padx=5, pady=5)
Label(root, text="(String)", bg="lightgrey").grid(row=14, column=3, padx=5, pady=5, sticky=W)

Label(root, text="PBA Metadata", bg="lightgrey").grid(row=15, column=1, padx=5, pady=5, sticky=W)
pm_entry = Entry(root, bd=1)
pm_entry.grid(row=15, column=2, padx=5, pady=5)
Label(root, text="(String)", bg="lightgrey").grid(row=15, column=3, padx=5, pady=5, sticky=W)

root.mainloop()

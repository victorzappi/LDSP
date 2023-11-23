import LDSPData from '/gui/js/LDSPData.js'
import LDSPControl from '/gui/js/LDSPControl.js'

export default class LDSP {
	constructor(ip = location.host.split(':')[0]) { // Splits ip from webserver port
		this.port = 5555
		this.addresses = {
			data:    'gui_data',
			control: 'gui_control'
		}
		this.ip = ip
		this.data    = new LDSPData    (this.port, this.addresses.data,    this.ip)
		this.control = new LDSPControl (this.port, this.addresses.control, this.ip)
	}
}

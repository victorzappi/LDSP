import LDSPData from './LDSPData.js'
//import BelaControl from './BelaControl.js'

export default class LDSP {
	constructor(ip=location.host) {
		this.port = 5555
		this.addresses = {
			data:    'gui_data',
			control: 'gui_control'
		}
		this.ip = ip
		this.data    = new LDSPData    (this.port, this.addresses.data,    this.ip)
		//this.control = new BelaControl (this.port, this.addresses.control, this.ip)
	}
}

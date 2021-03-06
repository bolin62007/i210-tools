/* This file is part of i2c-tools.
 *
 * i210-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * i210-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with i2c-tools. If not, see <http://www.gnu.org/licenses/>.
 */

#include "i210-flash.hpp"

namespace libi210 {

template <class T>
std::ostream& I210Common<T>::operator<< (std::ostream& stream) {


/*	stream << "Firmware:" << std::endl;
	stream << "test: " << std::hex << getFirmwareRevision() << std::endl;
	stream << "major: " << std::hex << (getFirmwareRevision() >> 8) << std::endl;
	stream << "minor: " << std::hex << (getFirmwareRevision() & 0xFF) << std::endl;
	stream << " Revision: " << std::to_string(getFirmwareRevision() >> 8) + "." + std::to_string(getFirmwareRevision() & 0xFF) << std::endl;
	*/
	stream << "Flash:" << std::endl;
	stream << " Size(KiB): " << std::to_string(getFlashSize()) << std::endl;
	stream << " Secure Mode: " << std::boolalpha << getSecureMode() << std::endl;
	stream << " Flash detected: " << std::boolalpha << getFlashDetected() << std::endl;
	stream << " Flash present: " << std::boolalpha << getFlashPresent() << std::endl;
	stream << " Flash in use: " << std::boolalpha << getFlashInUse() << std::endl;
	stream << " Flash speed(KHz): " << std::to_string(getFlashSpeed()) << std::endl;

	return stream;
}
template class I210Common<regmap::pci::MemMapped>;
template class I210Common<regmap::pci::IOMapped>;
template class I210Common<regmap::RegMapBase<regmap::RegBackendMemory>>;


template <class T>
uint16_t I210Common<T>::getFirmwareRevision() {

	std::uint16_t FirmwarePointer4K = m_oBackend.template getBackend().template get<std::uint16_t>(0x20000 + 0x20);
	//return FirmwarePointer4K >> 8;
	std::uint32_t FirmwareOffset = (std::uint32_t)(((FirmwarePointer4K>>8))) * 4096;

	//std::cout << "pointer: " << std::to_string(FirmwarePointer4K) << std::endl;
	//std::cout << "offset: " << std::to_string(FirmwareOffset) << std::endl;
	FirmwareOffset=0x2000;

	return m_oBackend.template getBackend().template get<std::uint16_t>(0x20000 + FirmwareOffset + ((256 + 8) * sizeof(std::uint16_t)));
}

template <class T>
uint32_t I210Common<T>::getFlashSize() {

	auto FLA = m_oBackend.template get<regmap::Register32_t>("FLA");

	// Flash size = 64KB * 2 ^ FL_SIZE
	return (1 << ((FLA & 0x000F0000) >> 17) << 6);
}

template <class T>
bool I210Common<T>::getSecureMode() {

	auto FLA = m_oBackend.template get<regmap::Register32_t>("FLA");
	return FLA.is_set("SECURE_MODE");
}

template <class T>
bool I210Common<T>::getFlashDetected() {

	auto EEC = m_oBackend.template get<regmap::Register32_t>("EEC");
	return EEC.is_set("FLASH_DETECTED");
}

template <class T>
bool I210Common<T>::getFlashPresent() {

	auto EEC = m_oBackend.template get<regmap::Register32_t>("EEC");
	return EEC.is_set("FLASH_PRESENT");
}

template <class T>
bool I210Common<T>::getFlashInUse() {

	auto EEC = m_oBackend.template get<regmap::Register32_t>("EEC");
	return EEC.is_set("FLASH_IN_USE");
}

template <class T>
uint32_t I210Common<T>::getFlashSpeed() {

	auto FLASHMODE = m_oBackend.template get<regmap::Register32_t>("FLASHMODE");

	if (FLASHMODE.is_set("SPEED_31MHz"))
		return 31250;
	if (FLASHMODE.is_set("SPEED_62MHz"))
		return 62500;

	return 15125;
}

template <class T>
FlashDump I210Common<T>::dump() {

	FlashDump buf(m_oBackend.template defFile(), 1048576);
	auto FLSWCNT = m_oBackend.template get<regmap::Register32_t>("FLSWCNT");
	auto FLSWCTL = m_oBackend.template get<regmap::Register32_t>("FLSWCTL");
	auto FLSWDATA = m_oBackend.template get<regmap::Register32_t>("FLSWDATA");

	for (std::uint32_t i = 0; i < buf.size(); i += 4) {

		// we're going to read 4 data bytes
		FLSWCNT = 0x4;

		// at flash address...
		FLSWCTL = 0x00000000 | i;

		// wait until the data is available in FLSWDATA
		FLSWCTL.wait(std::chrono::microseconds(1));

		// read the 4 bytes and store them
		std::uint32_t val = FLSWDATA;
		memcpy(static_cast<char*>(buf.data()) + i, &val, sizeof(val));
	}

	return buf;
}

};

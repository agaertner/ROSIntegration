#include "WebSocketConnection.h"

#include "ROSIntegrationCore.h"

#include <iomanip>
#include <Serialization/ArrayReader.h>

#include "WebSocketsModule.h"
#include "Containers/UnrealString.h"


bool WebSocketConnection::Init(std::string ip_addr, int port)
{
	UE_LOG(LogROS, Warning, TEXT("Using WebSocketConnection"));
	_sock = FWebSocketsModule::Get().CreateWebSocket(TEXT("ws://127.0.0.1:9090/rosbridge"));


	_sock->OnMessage().AddLambda([&](const FString& Message) -> void {

		UE_LOG(LogROS, Display, TEXT("in ws->OnMessage: %s"), *Message.Left(33));
		if (bson_only_mode_)
		{
			bson_t b;
			if (!bson_init_static(&b, reinterpret_cast<const uint8*>(*Message), Message.Len()))
			{
				UE_LOG(LogROS, Error, TEXT("Error on BSON parse - Ignoring message"));
				return;
			}
			if (incoming_message_callback_bson_)
			{
				incoming_message_callback_bson_(b);
			}
		}
		else
		{
			json j;
			j.Parse(TCHAR_TO_UTF8(*Message));

			if (_incoming_message_callback)
				_incoming_message_callback(j);
		}

		// This code will run when we receive a string message from the server.
		});

	_sock->Connect();
	
	// Setting up the receiver thread
	UE_LOG(LogROS, Display, TEXT("Setting up receiver thread ..."));


	return true;
}

bool WebSocketConnection::SendMessage(std::string data)
{
	// std::string msg = "{\"args\":{\"a\":1,\"b\":2},\"id\":\"call_service:/add_two_ints:23\",\"op\":\"call_service\",\"service\":\"/add_two_ints\"}";
	const uint8 *byte_msg = reinterpret_cast<const uint8*>(data.c_str());
	int32 bytes_sent = 0;
	// TODO check proper casting

	// TODO check errors on send
	_sock->Send(byte_msg, data.length(), true);
	UE_LOG(LogROS, VeryVerbose, TEXT("Send data: %s"), *FString(data.c_str()));
	UE_LOG(LogROS, Warning, TEXT("Send data: %s"), *FString(data.c_str()));

	return true;
}

bool WebSocketConnection::SendMessage(const uint8_t *data, unsigned int length)
{
	std::string s(reinterpret_cast<const char*>(data), length);

	auto zx = BytesToString(data, length);

	// FTCHARToUTF8 zz(*zx, zx.Len());
	FTCHARToUTF8 zz(reinterpret_cast<const TCHAR*>(data), length);
	FString toSend(TCHAR_TO_UTF8(s.c_str()));
	_sock->Send(data, length, false);
	// _sock->Send(*toSend, toSend.Len(), true);
	//_sock->Send(*zx, zx.Len(), true);
	// _sock->Send(zz.Get(), zz.Length(), true);

	/*
	uint8_t* zxc = new uint8_t[length+1];
	zxc[0] = '\xc2';
	for (unsigned int i = 0; i < length; ++i)
		zxc[i + 1] = data[i];
	// _sock->Send(data, length, true);
	//_sock->Send(zxc, length+1, true);
	delete[] zxc;
	*/
	UE_LOG(LogROS, Warning, TEXT("Send data[2.][%d]: %s"), length, *FString(length, s.c_str()));
	//UE_LOG(LogROS, Warning, TEXT("Send data[2.][%d]: %s"), length+1, zxc);

	// UE_LOG(LogROS, Warning, TEXT("Send data[2a][%d]: %s"), zx.Len(), *zx);
	// UE_LOG(LogROS, Warning, TEXT("Send data[2a][%d]: %s"), zz.Length(), zz.Get());
	// UE_LOG(LogROS, Warning, TEXT("Send data[x2a][%d]: %s"), FString(UTF8_TO_TCHAR(*toSend)).Len(), *FString(UTF8_TO_TCHAR(*toSend)));

	return true;
}
/*
int WebSocketConnection::ReceiverThreadFunction()
{
	TArray<uint8> binary_buffer;
	uint32_t buffer_size = 10 * 1024 * 1024;
	binary_buffer.Reserve(buffer_size);
	bool bson_state_read_length = true; // indicate that the receiver shall only get 4 bytes to start with
	int32_t bson_msg_length = 0;
	int32_t bson_msg_length_read = 0;
	int return_value = 0;

	while (run_receiver_thread) {

		ESocketConnectionState ConnectionState = _sock->GetConnectionState();
		if (ConnectionState != ESocketConnectionState::SCS_Connected) {
			if (ConnectionState == SCS_NotConnected) {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			} else {
				UE_LOG(LogROS, Error, TEXT("Error on connection"));
				ReportError(rosbridge2cpp::TransportError::R2C_SOCKET_ERROR);
				run_receiver_thread = false;
				return_value = 2; // error while receiving from socket
				continue;
			}
		}

		const FTimespan OneSecond(10 * 1000 * 1000);
		if (!_sock->Wait(ESocketWaitConditions::WaitForRead, OneSecond))
		{
			continue; // check if any errors occured
		}

		if (bson_only_mode_) {
			if (bson_state_read_length) {
				bson_msg_length_read = 0;
				binary_buffer.SetNumUninitialized(4, false);
				int32 bytes_read = 0;
				if (_sock->Recv(binary_buffer.GetData(), 4, bytes_read) && bytes_read > 0) {
					bson_msg_length_read += bytes_read;
					if (bytes_read == 4) {
#if PLATFORM_LITTLE_ENDIAN
						bson_msg_length = (
							binary_buffer.GetData()[3] << 24 |
							binary_buffer.GetData()[2] << 16 |
							binary_buffer.GetData()[1] << 8 |
							binary_buffer.GetData()[0]
						);
#else
						bson_msg_length = *((uint32_t*)&binary_buffer[0]);
#endif
						// Indicate the message retrieval mode
						bson_state_read_length = false;
						binary_buffer.SetNumUninitialized(bson_msg_length, false);
					} else {
						UE_LOG(LogROS, Error, TEXT("bytes_read is not 4 in bson_state_read_length==true. It's: %d"), bytes_read);
					}
				} else {
					UE_LOG(LogROS, Error, TEXT("Failed to recv(); Closing receiver thread."));
					run_receiver_thread = false;
				}
			} else {
				// Message retrieval mode
				int32 bytes_read = 0;
				if (_sock->Recv(binary_buffer.GetData() + bson_msg_length_read, bson_msg_length - bson_msg_length_read, bytes_read) && bytes_read > 0) {

					bson_msg_length_read += bytes_read;
					if (bson_msg_length_read == bson_msg_length) {
						// Full received message!
						bson_state_read_length = true;
						bson_t b;
						if (!bson_init_static(&b, binary_buffer.GetData(), bson_msg_length_read)) {
							UE_LOG(LogROS, Error, TEXT("Error on BSON parse - Ignoring message"));
							continue;
						}
						if (incoming_message_callback_bson_) {
							incoming_message_callback_bson_(b);
						}
					} else {
						UE_LOG(LogROS, VeryVerbose, TEXT("Binary buffer num is: %d"), binary_buffer.Num());
					}
				} else {
					UE_LOG(LogROS, Error, TEXT("Failed to recv()"));
				}
			}
		}
		else {
			FString result;
			uint32 count = 0;
			while (_sock->HasPendingData(count) && count > 0) {
				FArrayReader data;
				data.SetNumUninitialized(count);

				int32 bytes_read = 0;
				// read pending data into the Data array reader
				if (_sock->Recv(data.GetData(), data.Num(), bytes_read))
				{
					int32 dest_len = TStringConvert<ANSICHAR, TCHAR>::ConvertedLength((char*)(data.GetData()), data.Num());
					UE_LOG(LogROS, VeryVerbose, TEXT("count is %d"), count);
					UE_LOG(LogROS, VeryVerbose, TEXT("bytes_read is %d"), bytes_read);
					UE_LOG(LogROS, VeryVerbose, TEXT("dest_len will be %i"), dest_len);
					TCHAR* dest = new TCHAR[dest_len + 1];
					TStringConvert<ANSICHAR, TCHAR>::Convert(dest, dest_len, (char*)(data.GetData()), data.Num());
					dest[dest_len] = '\0';

					result += dest;

					delete[] dest;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			if (result.Len() == 0) {
				continue;
			}

			// TODO catch parse error properly
			// auto j = json::parse(received_data);
			json j;
			j.Parse(TCHAR_TO_UTF8(*result));

			if (_incoming_message_callback)
				_incoming_message_callback(j);
		}
	}

	return return_value;
}
*/

void WebSocketConnection::RegisterIncomingMessageCallback(std::function<void(json&)> fun)
{
	_incoming_message_callback = fun;
	_callback_function_defined = true;
}

void WebSocketConnection::RegisterIncomingMessageCallback(std::function<void(bson_t&)> fun)
{
	incoming_message_callback_bson_ = fun;
	_callback_function_defined = true;
}

void WebSocketConnection::RegisterErrorCallback(std::function<void(rosbridge2cpp::TransportError)> fun)
{
	_error_callback = fun;
}

void WebSocketConnection::ReportError(rosbridge2cpp::TransportError err)
{
	if (_error_callback)
		_error_callback(err);
}


void WebSocketConnection::SetTransportMode(rosbridge2cpp::ITransportLayer::TransportMode mode)
{
	switch (mode) {
	case rosbridge2cpp::ITransportLayer::JSON:
		bson_only_mode_ = false;
		break;
	case rosbridge2cpp::ITransportLayer::BSON:
		bson_only_mode_ = true;
		break;
	default:
		UE_LOG(LogROS, Error, TEXT("Given TransportMode not implemented!"));
	}
}

bool WebSocketConnection::IsHealthy() const
{
	return run_receiver_thread;
}

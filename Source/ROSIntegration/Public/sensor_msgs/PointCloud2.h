#pragma once 

#include "ROSBaseMsg.h"
#include "std_msgs/Header.h"

namespace ROSMessages {
	namespace sensor_msgs {
		class PointCloud2 : public FROSBaseMsg {
		public:

			// we use a local PointField definition here instead of sensor_msgs/PointField 
			// to avoid unecessary bloat by deriving from FROSBaseMsg and it is only used for PointCloud2 msg anyway
			struct PointField
			{
				enum EType
				{
					INT8 = 1,
					UINT8 = 2,
					INT16 = 3,
					UINT16 = 4,
					INT32 = 5,
					UINT32 = 6,
					FLOAT32 = 7,
					FLOAT64 = 8
				};

				FString name;
				uint32 offset;
				EType  datatype;
				uint32 count;
			};

			PointCloud2() 
				:data(nullptr)
			{
				_MessageType = "sensor_msgs/PointCloud2";
			}

			~PointCloud2() {
				if (data)
					delete[] data;
			}

			ROSMessages::std_msgs::Header header;

			uint32 height;
			uint32 width;

			TArray<PointField> fields;

			bool	is_bigendian;
			uint32	point_step;
			uint32	row_step;

			uint8* data;

			bool is_dense;
		};
	}
}

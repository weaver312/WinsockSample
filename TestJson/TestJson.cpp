#include "pch.h"
#include <iostream>
#include "cJSON.h"

int main()
{

}

//{
//	"name": "Awesome 4K",
//		"resolutions" : [
//	{
//		"width": 1280,
//			"height" : 720
//	},
//	{
//		"width": 1920,
//		"height" : 1080
//	},
//	{
//		"width": 3840,
//		"height" : 2160
//	}
//		]
//}

// cJSON使用步骤
// 1. 先创建根Object节点-cJSON_CreateObject();，这一步根节点只有个变量名，没有内容。字符串格式只有一对括号{}
// 2. 在object里创建string/number/array，通过cJSON_CreateXXX();
// 3. 向array里填充Object，以此类推
//    string/number都是属性值，"width: 1280", "name: sony"这种
//    object表示 { } ，节点
//    array表示多个object形成的列表属性， [ ] 这种
//    将属性添加到object里，用cJSON_AddItemToObject();
//    将array添加到object里，用cJSON_AddItemToObject();
//    将object添加到array里，用cJSON_AddItemToArray();
// 4. 用cJSON_Print相关函数写到文件里
// 5. 用cJSON_Parse读到内存里
// 这是先构建再添加
char* create_monitor(void)
{
	const unsigned int resolution_numbers[3][2] = {
		{1280, 720},
		{1920, 1080},
		{3840, 2160}
	};
	char *string = NULL;
	cJSON *name = NULL;
	cJSON *resolutions = NULL;
	cJSON *resolution = NULL;
	cJSON *width = NULL;
	cJSON *height = NULL;
	size_t index = 0;

	cJSON *monitor = cJSON_CreateObject();
	if (monitor == NULL)
	{
		goto end;
	}

	name = cJSON_CreateString("Awesome 4K");
	if (name == NULL)
	{
		goto end;
	}
	/* after creation was successful, immediately add it to the monitor,
	 * thereby transfering ownership of the pointer to it */
	cJSON_AddItemToObject(monitor, "name", name);

	resolutions = cJSON_CreateArray();
	if (resolutions == NULL)
	{
		goto end;
	}
	cJSON_AddItemToObject(monitor, "resolutions", resolutions);

	for (index = 0; index < (sizeof(resolution_numbers) / (2 * sizeof(int))); ++index)
	{
		resolution = cJSON_CreateObject();
		if (resolution == NULL)
		{
			goto end;
		}
		cJSON_AddItemToArray(resolutions, resolution);

		width = cJSON_CreateNumber(resolution_numbers[index][0]);
		if (width == NULL)
		{
			goto end;
		}
		cJSON_AddItemToObject(resolution, "width", width);

		height = cJSON_CreateNumber(resolution_numbers[index][1]);
		if (height == NULL)
		{
			goto end;
		}
		cJSON_AddItemToObject(resolution, "height", height);
	}

	string = cJSON_Print(monitor);
	if (string == NULL)
	{
		fprintf(stderr, "Failed to print monitor.\n");
	}

end:
	cJSON_Delete(monitor);
	return string;
}
// 这是直接添加。这两种各有所长，考虑到内存释放的不同场景
char *create_monitor_with_helpers(void)
{
	const unsigned int resolution_numbers[3][2] = {
		{1280, 720},
		{1920, 1080},
		{3840, 2160}
	};
	char *string = NULL;
	cJSON *resolutions = NULL;
	size_t index = 0;

	cJSON *monitor = cJSON_CreateObject();

	if (cJSON_AddStringToObject(monitor, "name", "Awesome 4K") == NULL)
	{
		goto end;
	}

	resolutions = cJSON_AddArrayToObject(monitor, "resolutions");
	if (resolutions == NULL)
	{
		goto end;
	}

	for (index = 0; index < (sizeof(resolution_numbers) / (2 * sizeof(int))); ++index)
	{
		cJSON *resolution = cJSON_CreateObject();

		if (cJSON_AddNumberToObject(resolution, "width", resolution_numbers[index][0]) == NULL)
		{
			goto end;
		}

		if (cJSON_AddNumberToObject(resolution, "height", resolution_numbers[index][1]) == NULL)
		{
			goto end;
		}

		cJSON_AddItemToArray(resolutions, resolution);
	}

	string = cJSON_Print(monitor);
	if (string == NULL) {
		fprintf(stderr, "Failed to print monitor.\n");
	}

end:
	cJSON_Delete(monitor);
	return string;
}

// 读取的例子，流程：
// 1. cJSON_Parse();返回一个cJSON结构体
// 2. cJSON_GetObjectItemCaseSensitive();获取字段或对象。默认函数不区分大小写
//    * 需要知道id名才能引用
// 3. cJSON_IsNumber();
//    cJSON_IsString();判断字段类型
//    cjson->valuedouble;
//    cjson->valuestring;获取字段值
// 4. 可以用 cJSON_ArrayForEach 这个宏遍历array
int supports_full_hd(const char * const monitor)
{
	const cJSON *resolution = NULL;
	const cJSON *resolutions = NULL;
	const cJSON *name = NULL;
	int status = 0;
	cJSON *monitor_json = cJSON_Parse(monitor);


	name = cJSON_GetObjectItemCaseSensitive(monitor_json, "name");
	if (cJSON_IsString(name) && (name->valuestring != NULL))
	{
		printf("Checking monitor \"%s\"\n", name->valuestring);
	}

	resolutions = cJSON_GetObjectItemCaseSensitive(monitor_json, "resolutions");
	cJSON_ArrayForEach(resolution, resolutions)
	{
		cJSON *width = cJSON_GetObjectItemCaseSensitive(resolution, "width");
		cJSON *height = cJSON_GetObjectItemCaseSensitive(resolution, "height");

		if (!cJSON_IsNumber(width) || !cJSON_IsNumber(height))
		{
			status = 0;
			goto end;
		}

		if ((width->valuedouble == 1920) && (height->valuedouble == 1080))
		{
			status = 1;
			goto end;
		}
	}

end:
	cJSON_Delete(monitor_json);
	return status;
}
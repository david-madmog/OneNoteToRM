#include "ONEPage.h"

using namespace Gdiplus;


void ONEPage::LoadPage(nlohmann::json PageJson) {
	sprintf_s(LogBuffer, LB_SIZE, "Creating Page");
	DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);



}



void ONEPage::DrawPage(void* DrawDetails) {
	;
}
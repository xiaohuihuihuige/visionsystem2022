#ifndef __MAT_SOURCE_H_
#define __MAT_SOURCE_H_

//#include <zxing\ZXing.h>
#include <opencv2\opencv.hpp>
#include <opencv2/core/core.hpp>

#include <fstream>
#include "zxing/LuminanceSource.h"
#include "zxing/common/Counted.h"
//#include "zxing/Reader.h"
//#include "zxing/common/GlobalHistogramBinarizer.h"
#include "zxing\common\HybridBinarizer.h"
#include "zxing/DecodeHints.h"
//#include "zxing/MultiFormatReader.h"
//#include "zxing/oned/CodaBarReader.h"
//#include "zxing/oned/Code39Reader.h"
//#include "zxing/oned/Code93Reader.h"
//#include "zxing/oned/Code128Reader.h"
//#include "zxing\oned\EAN13Reader.h"
#include "zxing\oned\MultiFormatOneDReader.h"

class MatSource : public zxing::LuminanceSource {
private:
    cv::Mat cvImage;

public:

	MatSource(cv::Mat &image): LuminanceSource(image.cols, image.rows)
	{
		cvImage = image.clone();
	}
	~MatSource()
	{
	}
	int getWidth() const { return cvImage.cols; }
	int getHeight() const { return cvImage.rows; }
    zxing::ArrayRef<char> getRow(int y, zxing::ArrayRef<char> row) const;
    zxing::ArrayRef<char> getMatrix() const;
    //static bool decode_by_zxing(cv::Mat& matsrc, std::string& code, std::string &codetype, cv::Mat &matresult);
/*#zxing解码库导入
INCLUDEPATH += $$quote( D:/Program Files/zxing-cpp-x64-vc15/include\ )
win32:CONFIG(release, debug|release): LIBS += -L'D:/Program Files/zxing-cpp-x64-vc15/lib/'\
-llibzxing
win32:CONFIG(debug, debug|release): LIBS += -L'D:/Program Files/zxing-cpp-x64-vc15/lib/'\
-llibzxing-debug*/


};

#endif /* __MAT_SOURCE_H_ */

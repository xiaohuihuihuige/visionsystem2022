#include "MatSource.h"
#include <zxing/common/IllegalArgumentException.h>

zxing::ArrayRef<char> MatSource::getRow(int y, zxing::ArrayRef<char> row) const {

	int width_ = getWidth();
	if (!row)
		row = zxing::ArrayRef<char>(width_);
	const char *p = cvImage.ptr<char>(y);
	for (int x = 0; x<width_; ++x, ++p)
		row[x] = *p;
	return row;
}

zxing::ArrayRef<char> MatSource::getMatrix() const {

	int width_ = getWidth();
	int height_ = getHeight();
	zxing::ArrayRef<char> matrix = zxing::ArrayRef<char>(width_*height_);
	for (int y = 0; y < height_; ++y)
	{
		const char *p = cvImage.ptr<char>(y);
		for (int x = 0; x < width_; ++x, ++p)
		{
			matrix[y*width_ + x] = *p;
		}
	}
	return matrix;
}
/*bool MatSource::decode_by_zxing(cv::Mat &matsrc, std::string &code, std::string &codetype, cv::Mat &matresult)
{
    const char* my_code_type[] = {
        "NONE",
        "AZTEC",
        "CODABAR",
        "CODE_39",
        "CODE_93",
        "CODE_128",
        "DATA_MATRIX",
        "EAN_8",
        "EAN_13",
        "ITF"
        "MAXICODE",
        "PDF_417",
        "QR_CODE",
        "RSS_14",
        "RSS_EXPANDED",
        "UPC_A",
        "UPC_E",
        "UPC_EAN_EXTENSION"
    };
    int channels = matsrc.channels();
    cv::Mat matbgr, matgray;
    if (channels==1)
    {
        cv::cvtColor(matsrc, matbgr,cv::COLOR_GRAY2BGR);
        matgray = matsrc.clone();
    }
    else if(channels==3)
    {
        cv::cvtColor(matsrc, matgray, cv::COLOR_BGR2GRAY);
        matbgr = matsrc.clone();
    }
    zxing::Ref<zxing::LuminanceSource> source(new MatSource(matgray));
    int width = source->getWidth();
    int height = source->getHeight();
    //fprintf(stderr, "image width: %d, height: %d\n", width, height);
    zxing::oned::MultiFormatOneDReader reader(zxing::DecodeHints::DEFAULT_HINT);
    zxing::Ref<zxing::Binarizer> binarizer(new zxing::HybridBinarizer(source));
    zxing::Ref<zxing::BinaryBitmap> bitmap(new zxing::BinaryBitmap(binarizer));
    try
    {
        zxing::Ref<zxing::Result> result(reader.decode(bitmap, zxing::DecodeHints(zxing::DecodeHints::DEFAULT_HINT)));
        code = result->getText()->getText();
        codetype = my_code_type[result->getBarcodeFormat().value];

        zxing::ArrayRef< zxing::Ref<zxing::ResultPoint> > rpoints = result->getResultPoints();
        cv::Point point0(rpoints[0]->getX() + 20, rpoints[0]->getY() + 20);
        cv::Point point1(rpoints[1]->getX() - 20, rpoints[1]->getY() - 20);
        cv::putText(matbgr, codetype, cv::Point(20, 20), 1, 2, cv::Scalar(0, 0, 255));
        cv::putText(matbgr, code, cv::Point(20, 40), 1, 2, cv::Scalar(0, 0, 255));
        cv::rectangle(matbgr, cv::Rect(point1, point0), cv::Scalar(0, 0, 255), 2);
        matresult = matbgr.clone();
        return true;
    }
    catch (zxing::Exception& e)
    {
        return false;
    }
}*/

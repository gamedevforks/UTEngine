#pragma once

class UModel;
class BspSurface;

class Shadowmap
{
public:
	void Load(UModel* model, const BspSurface& surface, int lightindex);

	int Width() const { return width; }
	int Height() const { return height; }
	const float* Pixels() const { return pixels.data(); }

private:
	int width = 0;
	int height = 0;
	std::vector<float> pixels;
	std::vector<float> tempbuf;
};

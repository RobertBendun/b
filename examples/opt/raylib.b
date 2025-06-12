main() {
	extrn InitWindow, CloseWindow, WindowShouldClose, SetTargetFPS,
		BeginDrawing, EndDrawing,
		ClearBackground, DrawRectangle;

	auto x, y, dx, dy;
	dx = dy = 1;
	x = y = 10;

	InitWindow(800, 600, "Raylib from B");
	SetTargetFPS(60);


	while (WindowShouldClose() == 0) {
		if (x+100 >= 800) dx *= -1;
		if (x     <=   0) dx *= -1;
		if (y+60  >= 600) dy *= -1;
		if (y     <=   0) dy *= -1;

		x += dx * 2;
		y += dy * 2;

		BeginDrawing();
		ClearBackground(0);
		DrawRectangle(x, y, 100, 60, 0xff80ff00);
		EndDrawing();
	}

	CloseWindow();
}

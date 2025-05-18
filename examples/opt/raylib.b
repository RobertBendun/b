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
		/* TODO: Unary - */
		if (x+100 >= 800) dx = (0 - 1) * dx;
		if (x <=   0) dx = (0 - 1) * dx;
		if (y+60 >= 600) dy = (0 - 1) * dy;
		if (y <=   0) dy = (0 - 1) * dy;

		/* TODO: Compund assigment += */
		x = x + dx * 2;
		y = y + dy * 2;

		BeginDrawing();
		ClearBackground(0);
		DrawRectangle(x, y, 100, 60, 4278255360);
		EndDrawing();
	}

	CloseWindow();
}

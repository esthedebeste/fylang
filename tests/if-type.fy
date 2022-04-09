const a = 3
let b = 5
fun main() {
	if(type typeof(a) == typeof(b)) // explicit type check
		if(typeof(b) == typeof(a))  // implicit type check
			0
		else 1
	else 1
}
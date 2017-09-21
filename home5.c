#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/export.h>
#include <linux/printk.h>
#include <linux/typecheck.h>
#include <linux/slab.h>
#include <linux/rbtree.h>
#include <linux/types.h>

MODULE_AUTHOR("Denis Suprunenko <chipsoft@gmail.com>");
MODULE_DESCRIPTION("Home (5) (rbtree) in Linux Kernel Training");
MODULE_LICENSE("Dual BSD/GPL");

struct srange {
	u32 start;
	u32 stop;
};

struct ranges_node {
	struct rb_node node;
	struct srange* range;
};

/* Define rbtree root node globally */
static struct rb_root root = RB_ROOT;

/* This is just to avoid code duplication.
 * Returns @node, @parent and @val initialized.
 */

#define __FIND_RANGE_BY_NUM(num, root, node, parent, val)			\
	do {															\
		/* Let's perform type checking using helpers				\
		 * from <linux/typecheck.h>.								\
		 */															\
		 typecheck(u32, num);										\
		 typecheck(struct rb_root *, root);							\
		 typecheck(struct rb_node **, node);						\
		 typecheck(struct rb_node *, parent);						\
		 typecheck(struct ranges_node *, val);						\
																 	\
		 node = &root->rb_node;										\
		 parent = NULL;												\
																 	\
		 while (*node) {											\
			 parent = *node;										\
			 val = rb_entry(parent, struct ranges_node, node);		\
			 if (num < val->range->start)							\
				 node = &parent->rb_left;							\
			 else if (num > val->range->stop)						\
				 node = &parent->rb_right;							\
			 else													\
				 break;												\
		 }															\
		 if (!*node)												\
			 val = NULL;											\
	} while (0)

/* This is just to avoid code duplication.
 * Returns @node, @parent and @val initialized.
 */
#define __FIND_MY_RANGE(rng, root, node, parent, val)			\
	do {														\
																\
		/* Let's perform type checking using helpers			\
		 * from <linux/typecheck.h>.							\
		 */														\
		typecheck(const struct srange *, rng);					\
		typecheck(struct rb_root *, root);						\
		typecheck(struct rb_node **, node);						\
		typecheck(struct rb_node *, parent);					\
		typecheck(struct ranges_node *, val);					\
																\
		node = &root->rb_node;									\
		parent = NULL;											\
																\
		while (*node) {											\
																\
																\
			parent = *node;										\
			val = rb_entry(parent, struct ranges_node, node);	\
			if (rng->start < val->range->start)					\
				node = &parent->rb_left;						\
			else if (rng->stop > val->range->stop)				\
				node = &parent->rb_right;						\
			else 												\
				break;											\
		}														\
		if (!*node)												\
			val = NULL;											\
	} while (0);												

struct ranges_node *add_my_range(const struct srange *rng, struct rb_root *root)
{
	struct rb_node **node, *parent;
	struct ranges_node *val, *val2;

	// Если нашли такой диапазон, то возвращаем его
	__FIND_MY_RANGE(rng, root, node, parent, val);
	if (val)
		 return val;
		 
	// Пытаемся найти возможные варианты объединения диапазонов
	__FIND_RANGE_BY_NUM(rng->start - 1, root, node, parent, val);
	__FIND_RANGE_BY_NUM(rng->stop + 1, root, node, parent, val2);

	//TODO: Предусмотреть, что добавляемый диапазон поглощает другие, и тогда такие диапазоны нужно удалить

	if (val && val2 && val == val2)
		return val; // Добавляемый диапазон находится внутри другого диапазона, поэтому не добавляем его

	if (!val && val2) {
		val2->range->start = rng->start; // Расширяем диапазон
		//rb_insert_color(&val2->node, root);
		return val2; // Возвращаем расширенный диапазон
	}

	if (val && !val2) {
		val->range->stop = rng->stop; // Расширяем диапазон
		//rb_insert_color(&val->node, root);
		return val; // Возвращаем расширенный диапазон
	}

	// Если ничего не подошло, то добавляем новый диапазон

	val = kmalloc(sizeof(*val), GFP_KERNEL); // Выделяем память для указателя
	if (!val)
		goto errout;

	val->range = kmalloc(sizeof(struct srange), GFP_KERNEL); // Выделяем память для диапазона
	if (!val)
		goto errout;

	if (!val || !val->range) goto errout_kfree_val;
	memcpy(val->range, rng, sizeof(struct srange));

	rb_link_node(&val->node, parent, node);
	rb_insert_color(&val->node, root);

	return val;

errout_kfree_val:
	kfree(val);
errout:
	return NULL;
}
EXPORT_SYMBOL(add_my_range);

void del_my_range(const struct srange *rng, struct rb_root *root)
{
	struct rb_node **node, *parent;
	struct ranges_node *val;

	__FIND_MY_RANGE(rng, root, node, parent, val);
	if (val && val->range->start == rng->start && val->range->stop == rng->stop) {
		rb_erase(&val->node, root);
		kfree(val->range);
		kfree(val);
	}
}
EXPORT_SYMBOL(del_my_range);

struct ranges_node *find_my_range(const struct srange *rng, struct rb_root *root)
{
	struct rb_node **node, *parent;
	struct ranges_node *val;

	__FIND_MY_RANGE(rng, root, node, parent, val);
	return val;
}
EXPORT_SYMBOL(find_my_range);

// Осуществляем поиск интервала по числу
struct ranges_node *find_range_by_num(u32 num,  struct rb_root *root)
{
	struct rb_node **node, *parent;
	struct ranges_node *val;

	__FIND_RANGE_BY_NUM(num, root, node, parent, val);
	return val;
}
EXPORT_SYMBOL(find_range_by_num);

void flush_tree(struct rb_root *root)
{
	struct ranges_node *val, *tmp;

	rbtree_postorder_for_each_entry_safe(val, tmp, root, node) {
		kfree(val->range);
		kfree(val);
	}

	*root = RB_ROOT;
}
EXPORT_SYMBOL(flush_tree);

static void print_tree(struct rb_root *root)
{
	struct ranges_node *val, *tmp;

	rbtree_postorder_for_each_entry_safe(val, tmp, root, node) {
		pr_info("val(%p): [%d - %d]\n",
			val, val->range->start, val->range->stop);
	}
}

static struct srange range1 = {0, 5};
static struct srange range2 = {1, 7};
static struct srange range3 = {2, 9};
static struct srange range4 = {10, 15};
static struct srange range5 = {20, 25};

static int __init test5_init(void)
{
	struct ranges_node *sn, *tmp;

	// Show ranges to add
	pr_info("Our ranges:\n");
	pr_info("#1 - [0..5]\n");
	pr_info("#2 - [1..7]\n");
	pr_info("#3 - [2..9]\n");
	pr_info("#4 - [10..15]\n");
	pr_info("#5 - [20..25]\n\n");

	/* 1 */
	sn = add_my_range(&range1, &root);
	BUG_ON(!sn);

	/* 2 */
	sn = add_my_range(&range2, &root);
	BUG_ON(!sn);

	/* 3 */
	sn = add_my_range(&range3, &root);
	BUG_ON(!sn);

	/* Ensure we not add already existing range
	 * and just return it's node.
	 */
	tmp = add_my_range(&range3, &root);
	BUG_ON(!tmp);
	BUG_ON(sn != tmp);

	pr_info("\nAfter add first 3 ranges: \n");
	print_tree(&root);

	/* Now delete range and make sure it is
	 * actually deleted. */
	del_my_range(&range3, &root);
	tmp = find_my_range(&range3, &root);
	BUG_ON(!tmp); // Диапазон не удалили, т.к. он входит внутрь другого диапазона

	pr_info("\nAfter del range #3\n");
	print_tree(&root);

	/* Add range again. */
	sn = add_my_range(&range3, &root);
	BUG_ON(!sn);

	pr_info("\nAfter add range #3 again\n");
	print_tree(&root);

	/* Attempt to delete non-existent range. */
	del_my_range(&range4, &root);

	/* Lookup for non-existent entry. */
	sn = find_my_range(&range4, &root);
	BUG_ON(sn);

	/* Lookup for existing entry. */
	sn = find_my_range(&range3, &root);
	BUG_ON(!sn);

	/* Add more ranges to array. */
	sn = add_my_range(&range4, &root);
	BUG_ON(!sn);
	sn = add_my_range(&range5, &root);
	BUG_ON(!sn);

	/* Now delete range and make sure it is
	 * actually deleted. */
	 del_my_range(&range5, &root);
	 tmp = find_my_range(&range5, &root);
	 BUG_ON(tmp);

	 /* Add again */
	 sn = add_my_range(&range5, &root);
	 BUG_ON(!sn);
 
	pr_info("\nResult tree after all add:\n");
	/* Print tree. */
	print_tree(&root);

	// Looking for range, that contains 11
	u32 val_to_find = 11; 
	sn = find_range_by_num(val_to_find, &root);
	BUG_ON(!sn);
	pr_info("\nFind range, that contain %d: [%d - %d]\n", val_to_find, sn->range->start, sn->range->stop);
	pr_info("\nThat's all, folks!\n\n");
	
	return 0;
}

static void __exit test5_exit(void)
{
	struct ranges_node *sn, *tmp;

	/* Flush entire tree. */
	flush_tree(&root);

	/* Now try to find known existing value. */
	sn = find_my_range(&range3, &root);
	BUG_ON(sn);

	/* Add range again and make sure it
	 * is really added.
	 */
	sn = add_my_range(&range3, &root);
	BUG_ON(!sn);

	tmp = find_my_range(&range3, &root);
	BUG_ON(!tmp);
	BUG_ON(sn != tmp);

	/* Flush entire tree finally. */
	flush_tree(&root);
}

module_init(test5_init);
module_exit(test5_exit);

# Generated by Django 5.2.1 on 2025-05-24 15:54

import django.db.models.deletion
from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ("tracker", "0005_remove_measurement_id_alter_measurement_timestamp"),
    ]

    operations = [
        migrations.AlterField(
            model_name="measurement",
            name="penguin",
            field=models.ForeignKey(
                blank=True,
                null=True,
                on_delete=django.db.models.deletion.CASCADE,
                related_name="measurements",
                to="tracker.taggedpenguin",
            ),
        ),
    ]

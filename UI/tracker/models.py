from django.db import models
from django.core.validators import MinValueValidator


class TaggedPenguin(models.Model):
    """Represents a penguin with an RFID tag."""
    id = models.CharField(
        max_length=50, primary_key=True,verbose_name="RFID Tag ID"
    )

    name = models.CharField(
        max_length=50, verbose_name="Penguin Name",
        null=True, blank=True
    )

    class Meta:
        verbose_name = "Tagged Penguin"
        verbose_name_plural = "Tagged Penguins"
        ordering = ["id"]
    
    def __str__(self):
        return f"{self.id}"

class Measurement(models.Model):
    """Records individial measurements of tagged penguins."""
    penguin = models.ForeignKey(
        TaggedPenguin,
        on_delete=models.CASCADE,
        related_name='measurements',
        null = True,
        blank = True
    )

    weight = models.DecimalField(
        max_digits=8,
        decimal_places=2,
        null=True,
        blank=True,
        validators=[MinValueValidator(0)],
        help_text="Weight in kilograms"
    )

    image = models.ImageField(
        upload_to="p_images/",
        verbose_name="Penguin Image"
    )

    timestamp = models.DateTimeField(
        primary_key=True,
        verbose_name="Measurement Time",
        help_text="Exact time from microcontroller (Local time)"
    )

    SOURCE_CHOICES = [
        ('manual', 'Manual'),
        ('automatic', 'Automatic'),
    ]

    source = models.CharField(
        max_length=10,
        choices=SOURCE_CHOICES,
        default='automatic',
        help_text="Source of the measurement"
    )

    class Meta:
        ordering = ['-timestamp']
        constraints = [
            models.UniqueConstraint(
                fields=['penguin', 'timestamp'],
                name='unique_measurement_per_penguin_time'
            )
        ]
    
    def __str__(self):
        return f"Measurement: {self.timestamp}"